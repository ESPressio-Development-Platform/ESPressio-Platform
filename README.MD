# ESPressio-Platform

`ESPressio-Platform` defines the clean-slate portable platform contracts and compile-time platform composition model for the ESPressio Development Platform.

This repository does **not** preserve or mirror legacy platform contracts from existing ESPressio libraries. Existing consumers will be migrated later to the Platform contracts once their current redesign work is complete.

## Capability model

Platform composition is static. Missing required capabilities, conflicting exclusive providers and provider dependency errors are compile-time failures rather than boot-time discovery.

```cpp
using namespace ESPressio::Platform;

struct MyBackend final : Backend {};

struct ExecutionProvider final
    : ProviderDeclaration<
          MyBackend,
          CapabilitySet<Capability::Execution,
                        Capability::ProcessorAffinity>> {};

struct ClockProvider final
    : ProviderDeclaration<
          MyBackend,
          CapabilitySet<Capability::Clock,
                        Capability::MonotonicClock,
                        Capability::HighResolutionClock>> {};

using MyPlatform = Composition<ExecutionProvider, ClockProvider>;

static_assert(MyPlatform::Provides<Capability::Execution>);
static_assert(MyPlatform::Satisfies<
    RequirementSet<Capability::Execution,
                   Capability::HighResolutionClock>>);
```

A consumer can make its requirement mandatory with `RequireT`:

```cpp
using Required = RequirementSet<Capability::Execution,
                                Capability::HighResolutionClock>;
using ValidatedPlatform = RequireT<MyPlatform, Required>;
```

If the composition does not satisfy `Required`, compilation fails.

## Exclusive services and shared features

Capabilities are intentionally split into two categories:

- `ExclusiveCapability`: a service for which exactly zero or one provider may be selected in a composition. If two providers claim the same exclusive capability, compilation fails.
- `SharedCapability`: a feature fact that may legitimately be reported by multiple providers.

The initial generic platform vocabulary includes exclusive service capabilities for execution, synchronization, queueing, memory, clock, entropy, GPIO and byte streams. Shared feature capabilities currently include processor affinity, interrupt-safe signalling, external memory, monotonic clocks and high-resolution clocks.

The capability vocabulary is intentionally extensible. Future backend and domain libraries can define additional capability types without modifying a central enum.

## Provider requirements

A provider may itself require capabilities from the final composition:

```cpp
struct SynchronizationProvider final
    : ProviderDeclaration<
          MyBackend,
          CapabilitySet<Capability::Synchronization>,
          RequirementSet<Capability::Execution>> {};
```

A composition containing this provider but no execution provider is ill-formed at compile time.

## Backend libraries

Concrete platform implementations live in sibling repositories:

- `ESPressio-Platform-Arduino`
- `ESPressio-Platform-FreeRTOS`
- `ESPressio-Platform-ESP-IDF`

Those libraries declare distinct backend identities and will implement the contracts owned here. A single firmware composition may deliberately contain providers from more than one backend.

## Design rules

- Compile-time resolution is preferred whenever the composition can be known during the build.
- Runtime results are reserved for genuinely runtime facts such as driver initialization failure or transient hardware availability.
- No mutable "last provider wins" registry is part of the capability model.
- Provider declarations carry no runtime state and impose no inheritance shape beyond their static metadata.
- The Platform layer is independent of Arduino, ESP-IDF and FreeRTOS headers.
- Domain-specific contracts such as Wi-Fi and Radio remain owned by their domain libraries; their backend implementations may still participate in Platform composition later.

## Validation

`tests/run_native_tests.sh` compiles the positive C++17 test and also verifies that invalid compositions fail compilation for the intended reasons.
