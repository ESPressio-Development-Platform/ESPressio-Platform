#include "ESPressio_Platform.hpp"
using namespace ESPressio::Platform;

struct TestBackend final : Backend {};

struct SlowClock final
    : ProviderDeclaration<
          TestBackend,
          CapabilitySet<
              CapabilityProfile<
                  Capability::Clock,
                  PropertyValue<PropertyKey::ClockFrequencyHz, std::uint64_t{500'000}>>>> {};

using P = Composition<SlowClock>;
using NeedOneMHz = RequirementSet<
    CapabilityRequirement<
        Capability::Clock,
        PropertyAtLeast<PropertyKey::ClockFrequencyHz, std::uint64_t{1'000'000}>>>;

using MustFail = RequireT<P, NeedOneMHz>;
int main() { return sizeof(MustFail); }
