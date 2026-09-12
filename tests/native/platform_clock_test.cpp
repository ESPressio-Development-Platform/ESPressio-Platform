#include <cstdint>
#include <type_traits>

#include "ESPressio_Platform.hpp"

using namespace ESPressio::Platform;

namespace {

struct SyntheticRtcOrigin final : Backend {};

class ExternalOscillatorClock final
    : public Clock::MonotonicProviderDeclaration<
          SyntheticRtcOrigin,
          32'768ULL,
          16U,
          PropertySet<
              PropertyValue<PropertyKey::ClockResolutionNanoseconds,
                            30'518ULL>>,
          CapabilitySet<Capability::InterruptReadableClock>> {
public:
    explicit constexpr ExternalOscillatorClock(Clock::Tick value = 0U) noexcept
        : value_(value & Clock::CounterMask(16U)) {}

    Clock::Tick Now() const noexcept {
        return value_ & Clock::CounterMask(16U);
    }

    Clock::Tick NowFromInterrupt() const noexcept {
        return Now();
    }

private:
    Clock::Tick value_;
};

static_assert(Clock::IsClockSourceV<ExternalOscillatorClock>);
static_assert(Clock::IsInterruptReadableClockSourceV<ExternalOscillatorClock>);
static_assert(Clock::FrequencyHz<ExternalOscillatorClock> == 32'768ULL);
static_assert(Clock::CounterWidthBits<ExternalOscillatorClock> == 16U);
static_assert(Clock::HasResolution<ExternalOscillatorClock>);

using RtcPlatform = Composition<ExternalOscillatorClock>;

static_assert(RtcPlatform::Provides<Capability::Clock>);
static_assert(RtcPlatform::Provides<Capability::MonotonicClock>);
static_assert(RtcPlatform::Provides<Capability::InterruptReadableClock>);

using RtcRequirements = RequirementSet<
    Capability::MonotonicClock,
    Clock::MinimumFrequency<32'768ULL>,
    Clock::MinimumCounterWidth<16U>,
    Clock::MaximumResolution<31'000ULL>>;

static_assert(RtcPlatform::Satisfies<RtcRequirements>);
using ValidatedRtcPlatform = RequireT<RtcPlatform, RtcRequirements>;
static_assert(std::is_same_v<ValidatedRtcPlatform, RtcPlatform>);

static_assert(Clock::Elapsed<ExternalOscillatorClock>(65'530U, 4U) == 10U,
              "Clock elapsed arithmetic must handle declared counter wrap");

struct BadReturnClock final
    : Clock::MonotonicProviderDeclaration<
          SyntheticRtcOrigin,
          1'000ULL,
          32U> {
    std::uint32_t Now() const noexcept { return 0U; }
};

static_assert(!Clock::IsClockSourceV<BadReturnClock>,
              "Clock source contract requires Clock::Tick return type");

struct MissingInterruptMethodClock final
    : Clock::MonotonicProviderDeclaration<
          SyntheticRtcOrigin,
          1'000ULL,
          32U,
          PropertySet<>,
          CapabilitySet<Capability::InterruptReadableClock>> {
    Clock::Tick Now() const noexcept { return 0U; }
};

static_assert(Clock::IsClockSourceV<MissingInterruptMethodClock>);
static_assert(!Clock::IsInterruptReadableClockSourceV<MissingInterruptMethodClock>,
              "Interrupt-readable claim must be backed by NowFromInterrupt()");

} // namespace

int main() {
    ExternalOscillatorClock clock{123U};
    return clock.Now() == 123U && clock.NowFromInterrupt() == 123U ? 0 : 1;
}
