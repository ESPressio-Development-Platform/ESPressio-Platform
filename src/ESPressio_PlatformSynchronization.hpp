#pragma once

#include <type_traits>
#include <utility>

#include "ESPressio_PlatformCapabilities.hpp"

namespace ESPressio::Platform {

namespace Capability {

/** Synchronization primitives own all storage statically inside their objects. */
struct StaticSynchronizationStorage final : SharedCapability {};

/** Selected synchronization provider supplies a recursive mutex type. */
struct RecursiveMutex final : SharedCapability {};

} // namespace Capability

namespace Synchronization {

/** Result returned by interrupt-safe signalling operations. */
struct InterruptSignalResult {
    bool StateChanged{false};
    bool HigherPriorityWaiterWoken{false};
};

namespace Detail {

template <typename T, typename = void>
struct IsMutex : std::false_type {};

template <typename T>
struct IsMutex<
    T,
    std::void_t<
        decltype(std::declval<T&>().Lock()),
        decltype(std::declval<T&>().TryLock()),
        decltype(std::declval<T&>().Unlock())>>
    : std::bool_constant<
          std::is_same_v<decltype(std::declval<T&>().Lock()), void> &&
          std::is_same_v<decltype(std::declval<T&>().TryLock()), bool> &&
          std::is_same_v<decltype(std::declval<T&>().Unlock()), bool> &&
          noexcept(std::declval<T&>().Lock()) &&
          noexcept(std::declval<T&>().TryLock()) &&
          noexcept(std::declval<T&>().Unlock())> {};

template <typename T, typename = void>
struct IsSignal : std::false_type {};

template <typename T>
struct IsSignal<
    T,
    std::void_t<
        decltype(std::declval<T&>().Give()),
        decltype(std::declval<T&>().Wait()),
        decltype(std::declval<T&>().TryWait()),
        decltype(std::declval<T&>().Reset())>>
    : std::bool_constant<
          std::is_same_v<decltype(std::declval<T&>().Give()), void> &&
          std::is_same_v<decltype(std::declval<T&>().Wait()), void> &&
          std::is_same_v<decltype(std::declval<T&>().TryWait()), bool> &&
          std::is_same_v<decltype(std::declval<T&>().Reset()), void> &&
          noexcept(std::declval<T&>().Give()) &&
          noexcept(std::declval<T&>().Wait()) &&
          noexcept(std::declval<T&>().TryWait()) &&
          noexcept(std::declval<T&>().Reset())> {};

template <typename T, typename = void>
struct IsInterruptSignal : std::false_type {};

template <typename T>
struct IsInterruptSignal<
    T,
    std::void_t<decltype(std::declval<T&>().GiveFromInterrupt())>>
    : std::bool_constant<
          IsSignal<T>::value &&
          std::is_same_v<decltype(std::declval<T&>().GiveFromInterrupt()),
                         InterruptSignalResult> &&
          noexcept(std::declval<T&>().GiveFromInterrupt())> {};

template <typename TExtraCapabilities>
struct DeclarationBuilder;

template <typename... TExtraCapabilities>
struct DeclarationBuilder<CapabilitySet<TExtraCapabilities...>> {
    template <typename TOrigin, typename TRequirements>
    using Type = ESPressio::Platform::ProviderDeclaration<
        TOrigin,
        CapabilitySet<Capability::Synchronization, TExtraCapabilities...>,
        TRequirements>;
};

template <typename T, bool TRequired, typename = void>
struct RecursiveProviderSatisfied : std::bool_constant<!TRequired> {};

template <typename T>
struct RecursiveProviderSatisfied<T, true, std::void_t<typename T::RecursiveMutex>>
    : IsMutex<typename T::RecursiveMutex> {};

template <typename T, bool TRequired, typename = void>
struct InterruptSignalProviderSatisfied : std::bool_constant<!TRequired> {};

template <typename T>
struct InterruptSignalProviderSatisfied<T, true, std::void_t<typename T::Signal>>
    : IsInterruptSignal<typename T::Signal> {};

template <typename T, typename = void>
struct IsSynchronizationProvider : std::false_type {};

template <typename T>
struct IsSynchronizationProvider<
    T,
    std::void_t<
        typename T::PlatformCapabilities,
        typename T::Mutex,
        typename T::Signal>>
    : std::bool_constant<
          IsProviderV<T> &&
          T::PlatformCapabilities::template Contains<Capability::Synchronization> &&
          IsMutex<typename T::Mutex>::value &&
          IsSignal<typename T::Signal>::value &&
          RecursiveProviderSatisfied<
              T,
              T::PlatformCapabilities::template Contains<Capability::RecursiveMutex>>::value &&
          InterruptSignalProviderSatisfied<
              T,
              T::PlatformCapabilities::template Contains<Capability::InterruptSafeSignalling>>::value> {};

} // namespace Detail

template <typename TOrigin,
          typename TExtraCapabilities = CapabilitySet<>,
          typename TRequirements = RequirementSet<>>
struct ProviderDeclaration
    : Detail::DeclarationBuilder<TExtraCapabilities>::template Type<
          TOrigin, TRequirements> {};

template <typename T>
inline constexpr bool IsMutexV = Detail::IsMutex<T>::value;

template <typename T>
inline constexpr bool IsSignalV = Detail::IsSignal<T>::value;

template <typename T>
inline constexpr bool IsInterruptSignalV = Detail::IsInterruptSignal<T>::value;

template <typename T>
inline constexpr bool IsProviderV = Detail::IsSynchronizationProvider<T>::value;

} // namespace Synchronization

} // namespace ESPressio::Platform
