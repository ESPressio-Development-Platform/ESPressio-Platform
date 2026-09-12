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

## Backend libraries

Concrete platform implementations live in sibling repositories:

- `ESPressio-Platform-Arduino`
- `ESPressio-Platform-FreeRTOS`
- `ESPressio-Platform-ESP-IDF`

Those libraries declare distinct backend identities and will implement the contracts owned here. A single firmware composition may deliberately contain providers from more than one backend.

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

`tests/run_native_tests.sh` compiles the positive C++17 test and also verifies that invalid compositions fail compilation. Negative coverage includes:

- duplicate exclusive providers;
- missing provider dependencies;
- missing consumer capabilities;
- duplicate property declarations;
- provider-side quantitative requirements that are not met;
- consumer-side quantitative requirements that are not met.
