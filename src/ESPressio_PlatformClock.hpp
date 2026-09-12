#pragma once

#include <cstddef>
#include <cstdint>
#include <type_traits>
#include <utility>

#include "ESPressio_PlatformCapabilities.hpp"

namespace ESPressio::Platform {

namespace Capability {

/**
 * Indicates that a Clock provider exposes an interrupt-context read operation
 * in the same tick domain as its normal read operation.
 */
struct InterruptReadableClock final : SharedCapability {};

} // namespace Capability

namespace PropertyKey {

/**
 * Number of meaningful low-order bits returned by a Clock provider.
 *
 * Counter wrap is therefore modulo 2^ClockCounterWidthBits. A value of 64
 * means the complete Platform::Clock::Tick representation is significant.
 */
struct ClockCounterWidthBits final : Property<std::size_t> {};

/**
 * Smallest time interval, in nanoseconds, that the provider guarantees it can
 * distinguish. Providers must omit this property when the underlying platform
 * does not make a portable compile-time guarantee.
 */
struct ClockResolutionNanoseconds final : Property<std::uint64_t> {};

} // namespace PropertyKey

namespace Clock {

/**
 * Common storage type for Platform clock readings.
 *
 * A provider may use fewer than 64 meaningful bits; the exact width is
 * published through PropertyKey::ClockCounterWidthBits.
 */
using Tick = std::uint64_t;

/** Nanoseconds in one SI second. */
inline constexpr std::uint64_t NanosecondsPerSecond = 1'000'000'000ULL;

/**
 * Converts a fixed-frequency tick rate into a conservative integral
 * nanosecond resolution. The result is rounded up so it never advertises
 * stronger precision than the frequency can represent.
 */
constexpr std::uint64_t ResolutionNanosecondsForFrequency(
    std::uint64_t frequencyHz) noexcept {
    return frequencyHz == 0U
               ? 0U
               : (NanosecondsPerSecond / frequencyHz) +
                     ((NanosecondsPerSecond % frequencyHz) == 0U ? 0U : 1U);
}

namespace Detail {

template <typename TProperties>
constexpr bool HasValidMandatoryProperties() noexcept {
    if constexpr (!TProperties::template Contains<PropertyKey::ClockFrequencyHz> ||
                  !TProperties::template Contains<PropertyKey::ClockCounterWidthBits>) {
        return false;
    } else {
        constexpr auto frequency =
            TProperties::template Value<PropertyKey::ClockFrequencyHz>;
        constexpr auto width =
            TProperties::template Value<PropertyKey::ClockCounterWidthBits>;
        return frequency > 0U && width > 0U && width <= sizeof(Tick) * 8U;
    }
}

template <typename T, typename = void>
struct IsClockSource : std::false_type {};

template <typename T>
struct IsClockSource<
    T,
    std::void_t<typename T::PlatformCapabilities,
                decltype(std::declval<const T&>().Now())>>
    : std::bool_constant<
          IsProviderV<T> &&
          T::PlatformCapabilities::template Contains<Capability::Clock> &&
          std::is_same_v<decltype(std::declval<const T&>().Now()), Tick> &&
          noexcept(std::declval<const T&>().Now()) &&
          HasValidMandatoryProperties<
              typename T::PlatformCapabilities::template PropertiesFor<
                  Capability::Clock>>()> {};

template <typename T, typename = void>
struct IsInterruptReadableClockSource : std::false_type {};

template <typename T>
struct IsInterruptReadableClockSource<
    T,
    std::void_t<decltype(std::declval<const T&>().NowFromInterrupt())>>
    : std::bool_constant<
          IsClockSource<T>::value &&
          T::PlatformCapabilities::template Contains<
              Capability::InterruptReadableClock> &&
          std::is_same_v<
              decltype(std::declval<const T&>().NowFromInterrupt()),
              Tick> &&
          noexcept(std::declval<const T&>().NowFromInterrupt())> {};

template <typename TExtraProperties,
          typename TExtraCapabilities>
struct DeclarationBuilder;

template <typename... TExtraProperties,
          typename... TExtraCapabilities>
struct DeclarationBuilder<
    PropertySet<TExtraProperties...>,
    CapabilitySet<TExtraCapabilities...>> {

    template <typename TBackend,
              std::uint64_t TFrequencyHz,
              std::size_t TCounterWidthBits,
              typename TRequirements>
    using Type = ESPressio::Platform::ProviderDeclaration<
        TBackend,
        CapabilitySet<
            CapabilityProfile<
                Capability::Clock,
                PropertyValue<PropertyKey::ClockFrequencyHz, TFrequencyHz>,
                PropertyValue<PropertyKey::ClockCounterWidthBits,
                              TCounterWidthBits>,
                TExtraProperties...>,
            TExtraCapabilities...>,
        TRequirements>;
};

template <typename TExtraProperties,
          typename TExtraCapabilities>
struct MonotonicDeclarationBuilder;

template <typename... TExtraProperties,
          typename... TExtraCapabilities>
struct MonotonicDeclarationBuilder<
    PropertySet<TExtraProperties...>,
    CapabilitySet<TExtraCapabilities...>> {

    template <typename TBackend,
              std::uint64_t TFrequencyHz,
              std::size_t TCounterWidthBits,
              typename TRequirements>
    using Type = ESPressio::Platform::ProviderDeclaration<
        TBackend,
        CapabilitySet<
            CapabilityProfile<
                Capability::Clock,
                PropertyValue<PropertyKey::ClockFrequencyHz, TFrequencyHz>,
                PropertyValue<PropertyKey::ClockCounterWidthBits,
                              TCounterWidthBits>,
                TExtraProperties...>,
            Capability::MonotonicClock,
            TExtraCapabilities...>,
        TRequirements>;
};

} // namespace Detail

template <typename T>
inline constexpr bool IsClockSourceV = Detail::IsClockSource<T>::value;

template <typename T>
inline constexpr bool IsInterruptReadableClockSourceV =
    Detail::IsInterruptReadableClockSource<T>::value;

/**
 * Static declaration helper for a generic Clock provider.
 *
 * TFrequencyHz describes the tick scale: a delta of TFrequencyHz ticks
 * represents one second. TCounterWidthBits describes the native wrap width.
 * Extra properties and feature capabilities remain fully extensible.
 */
template <
    typename TBackend,
    std::uint64_t TFrequencyHz,
    std::size_t TCounterWidthBits,
    typename TExtraProperties = PropertySet<>,
    typename TExtraCapabilities = CapabilitySet<>,
    typename TRequirements = RequirementSet<>>
struct ProviderDeclaration
    : Detail::DeclarationBuilder<
          TExtraProperties,
          TExtraCapabilities>::template Type<
              TBackend,
              TFrequencyHz,
              TCounterWidthBits,
              TRequirements> {
    static_assert(TFrequencyHz > 0U,
                  "Clock frequency must be greater than zero");
    static_assert(TCounterWidthBits > 0U &&
                      TCounterWidthBits <= sizeof(Tick) * 8U,
                  "Clock counter width must be in the range 1..64 bits");
};

/**
 * Static declaration helper for a clock driven exclusively by elapsed time.
 *
 * "Monotonic" is scoped to one runtime epoch. The raw counter may wrap at the
 * declared counter width; it must not be adjusted by civil/wall-clock changes.
 */
template <
    typename TBackend,
    std::uint64_t TFrequencyHz,
    std::size_t TCounterWidthBits,
    typename TExtraProperties = PropertySet<>,
    typename TExtraCapabilities = CapabilitySet<>,
    typename TRequirements = RequirementSet<>>
struct MonotonicProviderDeclaration
    : Detail::MonotonicDeclarationBuilder<
          TExtraProperties,
          TExtraCapabilities>::template Type<
              TBackend,
              TFrequencyHz,
              TCounterWidthBits,
              TRequirements> {
    static_assert(TFrequencyHz > 0U,
                  "Clock frequency must be greater than zero");
    static_assert(TCounterWidthBits > 0U &&
                      TCounterWidthBits <= sizeof(Tick) * 8U,
                  "Clock counter width must be in the range 1..64 bits");
};

/** Compile-time tick frequency advertised by a Clock provider. */
template <typename TClock>
inline constexpr std::uint64_t FrequencyHz =
    TClock::PlatformCapabilities::template PropertiesFor<
        Capability::Clock>::template Value<PropertyKey::ClockFrequencyHz>;

/** Compile-time native wrap width advertised by a Clock provider. */
template <typename TClock>
inline constexpr std::size_t CounterWidthBits =
    TClock::PlatformCapabilities::template PropertiesFor<
        Capability::Clock>::template Value<PropertyKey::ClockCounterWidthBits>;

/** Whether a Clock provider publishes a guaranteed effective resolution. */
template <typename TClock>
inline constexpr bool HasResolution =
    TClock::PlatformCapabilities::template PropertiesFor<
        Capability::Clock>::template Contains<
            PropertyKey::ClockResolutionNanoseconds>;

/**
 * Returns the meaningful-bit mask for a clock counter width.
 */
constexpr Tick CounterMask(std::size_t widthBits) noexcept {
    if (widthBits >= sizeof(Tick) * 8U) {
        return ~Tick{0};
    }
    return (Tick{1} << widthBits) - Tick{1};
}

/**
 * Computes elapsed ticks using the provider's declared modular counter width.
 *
 * Correctly handles one wrap between the two samples. As with all finite
 * counters, a caller cannot infer that one or more complete additional wraps
 * occurred without sampling often enough or using a higher-level epoch
 * extension.
 */
constexpr Tick Elapsed(Tick earlier,
                       Tick later,
                       std::size_t widthBits) noexcept {
    return (later - earlier) & CounterMask(widthBits);
}

/** Provider-typed overload of Elapsed(). */
template <typename TClock>
constexpr Tick Elapsed(Tick earlier, Tick later) noexcept {
    static_assert(IsClockSourceV<TClock>,
                  "Elapsed<TClock> requires a valid Platform Clock source");
    return Elapsed(earlier, later, CounterWidthBits<TClock>);
}

/**
 * Compile-time Clock requirement with a minimum tick frequency.
 */
template <std::uint64_t TMinimumFrequencyHz>
using MinimumFrequency = CapabilityRequirement<
    Capability::Clock,
    PropertyAtLeast<PropertyKey::ClockFrequencyHz,
                    TMinimumFrequencyHz>>;

/**
 * Compile-time Clock requirement with a minimum counter width.
 */
template <std::size_t TMinimumCounterWidthBits>
using MinimumCounterWidth = CapabilityRequirement<
    Capability::Clock,
    PropertyAtLeast<PropertyKey::ClockCounterWidthBits,
                    TMinimumCounterWidthBits>>;

/**
 * Compile-time Clock requirement with a maximum effective resolution.
 *
 * Smaller resolution values are stronger. A provider that does not publish
 * ClockResolutionNanoseconds does not satisfy this requirement.
 */
template <std::uint64_t TMaximumResolutionNanoseconds>
using MaximumResolution = CapabilityRequirement<
    Capability::Clock,
    PropertyAtMost<PropertyKey::ClockResolutionNanoseconds,
                   TMaximumResolutionNanoseconds>>;

} // namespace Clock
} // namespace ESPressio::Platform
