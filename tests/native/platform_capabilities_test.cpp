#include <type_traits>

#include "ESPressio_Platform.hpp"

using namespace ESPressio::Platform;

namespace {

struct BackendA final : Backend {};
struct BackendB final : Backend {};

struct ExecutionProvider final
    : ProviderDeclaration<BackendA,
                          CapabilitySet<Capability::Execution,
                                        Capability::ProcessorAffinity>> {};

struct ClockProvider final
    : ProviderDeclaration<BackendB,
                          CapabilitySet<Capability::Clock,
                                        Capability::MonotonicClock,
                                        Capability::HighResolutionClock>> {};

struct SynchronizationProvider final
    : ProviderDeclaration<BackendA,
                          CapabilitySet<Capability::Synchronization,
                                        Capability::InterruptSafeSignalling>,
                          RequirementSet<Capability::Execution>> {};

struct DiagnosticProvider final
    : ProviderDeclaration<BackendB,
                          CapabilitySet<Capability::ProcessorAffinity>> {};

using TestPlatform = Composition<ExecutionProvider,
                                 ClockProvider,
                                 SynchronizationProvider,
                                 DiagnosticProvider>;

static_assert(TestPlatform::IsValid);
static_assert(TestPlatform::ProviderCount == 4U);
static_assert(TestPlatform::Provides<Capability::Execution>);
static_assert(TestPlatform::Provides<Capability::Synchronization>);
static_assert(TestPlatform::Provides<Capability::Clock>);
static_assert(!TestPlatform::Provides<Capability::GPIO>);

static_assert(TestPlatform::ProvidersFor<Capability::Execution> == 1U);
static_assert(TestPlatform::ProvidersFor<Capability::ProcessorAffinity> == 2U,
              "Shared feature capabilities may be supplied by more than one provider");

static_assert(std::is_same_v<TestPlatform::ProviderFor<Capability::Execution>,
                             ExecutionProvider>);

using Required = RequirementSet<Capability::Execution,
                                Capability::Synchronization,
                                Capability::HighResolutionClock>;
static_assert(TestPlatform::Satisfies<Required>);
using ValidatedForConsumer = RequireT<TestPlatform, Required>;
static_assert(std::is_same_v<ValidatedForConsumer, TestPlatform>);

} // namespace

int main() {
    return 0;
}
