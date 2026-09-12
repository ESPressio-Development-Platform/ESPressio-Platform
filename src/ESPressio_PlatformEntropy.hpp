#pragma once

#include <type_traits>
#include <utility>

#include "ESPressio_PlatformCapabilities.hpp"
#include "ESPressio_PlatformIO.hpp"

namespace ESPressio::Platform {

namespace Capability {

/** Provider is backed by a hardware random-number generator.
 *
 * This describes provenance only. It does not assert a fixed number of entropy
 * bits or that a continuous physical entropy source is active at runtime.
 */
struct HardwareRandomGenerator final : SharedCapability {};

} // namespace Capability

namespace Entropy {

namespace Detail {

template <typename TExtraCapabilities>
struct DeclarationBuilder;

template <typename... TExtraCapabilities>
struct DeclarationBuilder<CapabilitySet<TExtraCapabilities...>> {
    template <typename TOrigin, typename TRequirements>
    using Type = ESPressio::Platform::ProviderDeclaration<
        TOrigin,
        CapabilitySet<Capability::Entropy, TExtraCapabilities...>,
        TRequirements>;
};

template <typename T, typename = void>
struct IsSource : std::false_type {};

template <typename T>
struct IsSource<
    T,
    std::void_t<
        typename T::PlatformCapabilities,
        decltype(std::declval<T&>().Fill(std::declval<IO::MutableBuffer>()))>>
    : std::bool_constant<
          IsProviderV<T> &&
          T::PlatformCapabilities::template Contains<Capability::Entropy> &&
          std::is_same_v<decltype(std::declval<T&>().Fill(std::declval<IO::MutableBuffer>())),
                         IO::Result> &&
          noexcept(std::declval<T&>().Fill(std::declval<IO::MutableBuffer>()))> {};

} // namespace Detail

template <typename TOrigin,
          typename TExtraCapabilities = CapabilitySet<>,
          typename TRequirements = RequirementSet<>>
struct ProviderDeclaration
    : Detail::DeclarationBuilder<TExtraCapabilities>::template Type<
          TOrigin, TRequirements> {};

template <typename T>
inline constexpr bool IsSourceV = Detail::IsSource<T>::value;

} // namespace Entropy

} // namespace ESPressio::Platform
