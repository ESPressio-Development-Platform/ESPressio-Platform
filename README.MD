# ESPressio-Platform

`ESPressio-Platform` defines the clean-slate portable platform contracts and compile-time platform composition model for the ESPressio Development Platform.

This repository does **not** preserve or mirror legacy platform contracts from existing ESPressio libraries. Existing consumers will be migrated later to the Platform contracts once their current redesign work is complete.

## Capability model

Platform composition is static. Missing required capabilities, conflicting exclusive providers, provider dependency errors and unsatisfied capability-property constraints are compile-time failures rather than boot-time discovery.

```cpp
using namespace ESPressio::Platform;

struct MyBackend final : Backend {};

struct ExecutionProvider final
    : ProviderDeclaration<
          MyBackend,
          CapabilitySet<
              CapabilityProfile<
                  Capability::Execution,
                  PropertyValue<PropertyKey::ProcessorCount, std::size_t{2}>>,
              Capability::ProcessorAffinity>> {};

struct ClockProvider final
    : ProviderDeclaration<
          MyBackend,
          CapabilitySet<
              CapabilityProfile<
                  Capability::Clock,
                  PropertyValue<PropertyKey::ClockFrequencyHz,
                                std::uint64_t{10'000'000}>>,
              Capability::MonotonicClock,
              Capability::HighResolutionClock>> {};

using MyPlatform = Composition<ExecutionProvider, ClockProvider>;

static_assert(MyPlatform::Provides<Capability::Execution>);
static_assert(MyPlatform::PropertyValue<Capability::Execution,
                                        PropertyKey::ProcessorCount> == 2U);
```

## Capability properties and constraints

A capability can carry strongly typed compile-time properties. Properties belong to the capability they qualify; they are not global mutable platform state.

`ESPressio-Platform` provides initial property keys for processor count, clock frequency, guaranteed external-memory bytes and maximum queue depth. Property keys are open types, so future platform or domain libraries may define additional keys without changing a central enum.

```cpp
using FastClock =
    CapabilityRequirement<
        Capability::Clock,
        PropertyAtLeast<PropertyKey::ClockFrequencyHz,
                        std::uint64_t{1'000'000}>>;

using Required =
    RequirementSet<
        Capability::Execution,
        FastClock>;

using ValidatedPlatform = RequireT<MyPlatform, Required>;
```

`RequireT` fails compilation if the required capability is absent, if the required property is absent, or if the declared property value does not satisfy the constraint.

The generic constraint vocabulary currently includes:

- `PropertyEquals`
- `PropertyAtLeast`
- `PropertyAtMost`
- `PropertyGreaterThan`
- `PropertyLessThan`

A provider may also depend on a quantitative property of another provider:

```cpp
using DualCoreExecution =
    CapabilityRequirement<
        Capability::Execution,
        PropertyAtLeast<PropertyKey::ProcessorCount, std::size_t{2}>>;

struct SynchronizationProvider final
    : ProviderDeclaration<
          MyBackend,
          CapabilitySet<Capability::Synchronization>,
          RequirementSet<DualCoreExecution>> {};
```

A composition containing `SynchronizationProvider` and only a single-core execution provider is ill-formed at compile time.

## Exclusive services and shared features

Capabilities are intentionally split into two categories:

- `ExclusiveCapability`: a service for which exactly zero or one provider may be selected in a composition. If two providers claim the same exclusive capability, compilation fails.
- `SharedCapability`: a feature fact that may legitimately be reported by multiple providers.

The initial generic platform vocabulary includes exclusive service capabilities for execution, synchronization, queueing, memory, clock, entropy, GPIO and byte streams. Shared feature capabilities currently include processor affinity, interrupt-safe signalling, external memory, monotonic clocks and high-resolution clocks.

The capability vocabulary is intentionally extensible. Future backend and domain libraries can define additional capability types without modifying a central enum.

## Provider requirements

A provider may itself require capabilities or capability-property constraints from the final composition:

```cpp
struct SynchronizationProvider final
    : ProviderDeclaration<
          MyBackend,
          CapabilitySet<Capability::Synchronization>,
          RequirementSet<Capability::Execution>> {};
```

A composition containing this provider but no execution provider is ill-formed at compile time.

## Property inspection

For a capability with exactly one selected provider, the composition exposes the provider's properties directly:

```cpp
static_assert(MyPlatform::HasProperty<Capability::Clock,
                                      PropertyKey::ClockFrequencyHz>);

static_assert(MyPlatform::PropertyValue<Capability::Clock,
                                        PropertyKey::ClockFrequencyHz>
              == 10'000'000ULL);
```

`ProviderFor<Capability>` and direct property inspection deliberately require a unique provider. Shared capabilities may have multiple contributors; requirements against shared capabilities are satisfied when at least one contributing provider meets the requested constraints.

## Bus I/O and GPIO

Bus and GPIO contracts are deliberately multi-instance rather than global singletons.

`I2C::DeviceProviderDeclaration` represents a bound I2C endpoint identified by a device tag. Address, address width and requested bus frequency are compile-time properties. The portable endpoint contract provides write, read and repeated-START write/read operations over non-owning byte views.

`SPI::DeviceProviderDeclaration` similarly binds a device tag, frequency, mode and bit order. Its minimum contract is a full-duplex transfer operation. Chip-select is intentionally not imposed by the portable SPI contract, allowing software GPIO CS and backend-managed hardware CS to coexist.

GPIO providers are domain-tagged. `GPIO::DefaultDomain` maps to the generic `Capability::GPIO`, while additional domains use `Capability::GPIODomain<Tag>` so an MCU GPIO controller and one or more external expanders can coexist in a composition. Stronger GPIO behavior is expressed through separate feature capabilities such as pull-up, pull-down, open-drain, bidirectional operation and interrupts.

All three contracts distinguish compile-time structural support from runtime availability. A selected provider can satisfy a requirement statically while still reporting a runtime driver/device/resource failure through `IO::Result`.

See `docs/BUS_GPIO.md` for the detailed contract rationale.

## ByteStream

`ByteStream::ProviderDeclaration` defines a readable/writable byte-stream service with explicit availability and transfer counts. `ByteStream::DefaultStream` maps to the canonical `Capability::ByteStream`; additional tagged streams use `Capability::ByteStreamEndpoint<Tag>` so a serial console, modem, file-like transport, network client, or hardware-specific stream can coexist in one Platform composition.

The portable contract is deliberately small:

- `Available(std::size_t&)` reports bytes immediately available for reading;
- `Read(IO::MutableBuffer)` returns `IO::TransferResult`;
- `Write(IO::ConstBuffer)` returns `IO::TransferResult`.

A successful transfer may be partial. The returned count is always authoritative, and `IO::TransferResult::Completed(requested)` is provided when a caller requires an exact-length operation. The generic contract does not invent a universal timeout or non-blocking-write policy because those guarantees differ materially between stream implementations.

## Entropy

`Entropy::ProviderDeclaration` defines one selected entropy/random-byte source through `Fill(IO::MutableBuffer)`. Returning `IO::Result::Ok` means the entire requested buffer has been populated; partial entropy fills are not part of the generic contract.

`Capability::HardwareRandomGenerator` may additionally be advertised when a provider is physically backed by a hardware RNG. This feature describes provenance only. `ESPressio-Platform` intentionally defines no fixed entropy-bits or cryptographic-strength property because those guarantees may depend on runtime hardware state, initialization, or an active physical noise source. Hardware-specific libraries may implement the same Entropy contract without belonging to an Arduino, ESP-IDF, or FreeRTOS backend.

## Backend libraries

Concrete platform implementations live in sibling repositories:

- `ESPressio-Platform-Arduino`
- `ESPressio-Platform-FreeRTOS`
- `ESPressio-Platform-ESP-IDF`

Those libraries declare distinct backend identities and implement only contracts their underlying environment genuinely provides. A single firmware composition may deliberately contain providers from more than one backend.

## Design rules

- Compile-time resolution is preferred whenever the composition can be known during the build.
- Structural capability and configured quantitative limits are compile-time facts when the backend can declare them statically.
- Runtime results are reserved for genuinely runtime facts such as driver initialization failure, physical hardware availability or transient resource exhaustion.
- No mutable "last provider wins" registry is part of the capability model.
- Provider declarations carry no runtime state and impose no implementation shape beyond their static metadata.
- Capability properties are strongly typed and scoped to the capability they describe.
- The Platform layer is independent of Arduino, ESP-IDF and FreeRTOS headers.
- Domain-specific contracts such as Wi-Fi and Radio remain owned by their domain libraries; their backend implementations may still participate in Platform composition later.

## Validation

`tests/run_native_tests.sh` compiles the positive C++17 tests and also verifies that invalid compositions fail compilation. Negative coverage includes duplicate exclusive providers, missing provider dependencies, missing consumer capabilities, duplicate property declarations, unsatisfied quantitative requirements, duplicate providers for the same bound I2C endpoint, and duplicate providers for the same tagged ByteStream endpoint.
