#include <type_traits>

#include "ESPressio_Platform.hpp"

using namespace ESPressio::Platform;

namespace {

struct BackendA final : Backend {};
struct BackendB final : Backend {};

struct ExecutionProvider final
    : ProviderDeclaration<
          BackendA,
          CapabilitySet<
              CapabilityProfile<
                  Capability::Execution,
                  PropertyValue<PropertyKey::ProcessorCount, std::size_t{2}>>,
              Capability::ProcessorAffinity>> {};

struct ClockProvider final
    : ProviderDeclaration<
          BackendB,
          CapabilitySet<
              CapabilityProfile<
                  Capability::Clock,
                  PropertyValue<PropertyKey::ClockFrequencyHz, std::uint64_t{10'000'000}>>,
              Capability::MonotonicClock,
              Capability::HighResolutionClock>> {};

using DualCoreExecution =
    CapabilityRequirement<
        Capability::Execution,
        PropertyAtLeast<PropertyKey::ProcessorCount, std::size_t{2}>>;

struct SynchronizationProvider final
    : ProviderDeclaration<
          BackendA,
          CapabilitySet<Capability::Synchronization,
                        Capability::InterruptSafeSignalling>,
          RequirementSet<DualCoreExecution>> {};

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

static_assert(TestPlatform::HasProperty<Capability::Execution,
                                        PropertyKey::ProcessorCount>);
static_assert(TestPlatform::PropertyValue<Capability::Execution,
                                          PropertyKey::ProcessorCount> == 2U);
static_assert(TestPlatform::PropertyValue<Capability::Clock,
                                          PropertyKey::ClockFrequencyHz> == 10'000'000ULL);

using FastEnoughClock =
    CapabilityRequirement<
        Capability::Clock,
        PropertyAtLeast<PropertyKey::ClockFrequencyHz, std::uint64_t{1'000'000}>>;

using TooFastClock =
    CapabilityRequirement<
        Capability::Clock,
        PropertyAtLeast<PropertyKey::ClockFrequencyHz, std::uint64_t{20'000'000}>>;

using ExactlyTwoProcessors =
    CapabilityRequirement<
        Capability::Execution,
        PropertyEquals<PropertyKey::ProcessorCount, std::size_t{2}>>;

using AtMostFourProcessors =
    CapabilityRequirement<
        Capability::Execution,
        PropertyAtMost<PropertyKey::ProcessorCount, std::size_t{4}>>;

using MoreThanOneProcessor =
    CapabilityRequirement<
        Capability::Execution,
        PropertyGreaterThan<PropertyKey::ProcessorCount, std::size_t{1}>>;

using LessThanThreeProcessors =
    CapabilityRequirement<
        Capability::Execution,
        PropertyLessThan<PropertyKey::ProcessorCount, std::size_t{3}>>;

static_assert(TestPlatform::ProvidersSatisfying<FastEnoughClock> == 1U);
static_assert(TestPlatform::ProvidersSatisfying<TooFastClock> == 0U);

using Required = RequirementSet<
    Capability::Execution,
    Capability::Synchronization,
    Capability::HighResolutionClock,
    FastEnoughClock>;
static_assert(TestPlatform::Satisfies<Required>);
using ValidatedForConsumer = RequireT<TestPlatform, Required>;
static_assert(std::is_same_v<ValidatedForConsumer, TestPlatform>);

static_assert(TestPlatform::Satisfies<RequirementSet<ExactlyTwoProcessors>>);
static_assert(TestPlatform::Satisfies<RequirementSet<AtMostFourProcessors>>);
static_assert(TestPlatform::Satisfies<RequirementSet<MoreThanOneProcessor>>);
static_assert(TestPlatform::Satisfies<RequirementSet<LessThanThreeProcessors>>);
static_assert(!TestPlatform::Satisfies<RequirementSet<TooFastClock>>);

struct DMAAlignmentBytes final : Property<std::size_t> {};

struct MemoryProvider final
    : ProviderDeclaration<
          BackendB,
          CapabilitySet<
              CapabilityProfile<
                  Capability::Memory,
                  PropertyValue<DMAAlignmentBytes, std::size_t{16}>>>> {};

using MemoryPlatform = Composition<MemoryProvider>;
static_assert(MemoryPlatform::PropertyValue<Capability::Memory,
                                            DMAAlignmentBytes> == 16U);

} // namespace

int main() {
    return 0;
}
