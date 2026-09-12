#include "ESPressio_Platform.hpp"

using namespace ESPressio::Platform;

struct Origin final : Backend {};

struct CoarseClock final
    : Clock::MonotonicProviderDeclaration<
          Origin,
          1'000ULL,
          32U,
          PropertySet<
              PropertyValue<PropertyKey::ClockResolutionNanoseconds,
                            1'000'000ULL>>> {
    Clock::Tick Now() const noexcept { return 0U; }
};

using Platform = Composition<CoarseClock>;
using Requirements = RequirementSet<Clock::MaximumResolution<1'000ULL>>;
using MustFail = RequireT<Platform, Requirements>;

int main() { return sizeof(MustFail); }
