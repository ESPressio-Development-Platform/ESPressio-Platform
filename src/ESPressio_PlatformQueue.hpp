#pragma once

#include <cstddef>
#include <cstdint>
#include <type_traits>
#include <utility>

#include "ESPressio_PlatformCapabilities.hpp"

namespace ESPressio::Platform {

namespace Capability {

/** Queue instances own their control/payload storage statically. */
struct StaticQueueStorage final : SharedCapability {};

/** Selected queue provider supports sending from interrupt context. */
struct QueueSendFromInterrupt final : SharedCapability {};

/** Selected queue provider supports receiving from interrupt context. */
struct QueueReceiveFromInterrupt final : SharedCapability {};

} // namespace Capability

namespace Queue {

/** Result returned by interrupt-safe queue operations. */
struct InterruptOperationResult {
    bool Succeeded{false};
    bool HigherPriorityWaiterWoken{false};
};

namespace Detail {

template <typename TQueue, typename TElement, std::size_t TDepth, typename = void>
struct IsQueue : std::false_type {};

template <typename TQueue, typename TElement, std::size_t TDepth>
struct IsQueue<
    TQueue,
    TElement,
    TDepth,
    std::void_t<
        decltype(TQueue::Depth),
        decltype(TQueue::ElementSizeBytes),
        decltype(TQueue::PayloadStorageBytes),
        decltype(std::declval<TQueue&>().Send(std::declval<const TElement&>())),
        decltype(std::declval<TQueue&>().TrySend(std::declval<const TElement&>())),
        decltype(std::declval<TQueue&>().Receive(std::declval<TElement&>())),
        decltype(std::declval<TQueue&>().TryReceive(std::declval<TElement&>())),
        decltype(std::declval<const TQueue&>().Size()),
        decltype(std::declval<TQueue&>().Reset())>>
    : std::bool_constant<
          (TDepth > 0U) &&
          static_cast<std::size_t>(TQueue::Depth) == TDepth &&
          static_cast<std::size_t>(TQueue::ElementSizeBytes) == sizeof(TElement) &&
          static_cast<std::size_t>(TQueue::PayloadStorageBytes) == sizeof(TElement) * TDepth &&
          std::is_same_v<decltype(std::declval<TQueue&>().Send(std::declval<const TElement&>())), void> &&
          std::is_same_v<decltype(std::declval<TQueue&>().TrySend(std::declval<const TElement&>())), bool> &&
          std::is_same_v<decltype(std::declval<TQueue&>().Receive(std::declval<TElement&>())), void> &&
          std::is_same_v<decltype(std::declval<TQueue&>().TryReceive(std::declval<TElement&>())), bool> &&
          std::is_same_v<decltype(std::declval<const TQueue&>().Size()), std::size_t> &&
          std::is_same_v<decltype(std::declval<TQueue&>().Reset()), void> &&
          noexcept(std::declval<TQueue&>().Send(std::declval<const TElement&>())) &&
          noexcept(std::declval<TQueue&>().TrySend(std::declval<const TElement&>())) &&
          noexcept(std::declval<TQueue&>().Receive(std::declval<TElement&>())) &&
          noexcept(std::declval<TQueue&>().TryReceive(std::declval<TElement&>())) &&
          noexcept(std::declval<const TQueue&>().Size()) &&
          noexcept(std::declval<TQueue&>().Reset())> {};

template <typename TQueue, typename TElement, typename = void>
struct IsInterruptQueue : std::false_type {};

template <typename TQueue, typename TElement>
struct IsInterruptQueue<
    TQueue,
    TElement,
    std::void_t<
        decltype(std::declval<TQueue&>().SendFromInterrupt(std::declval<const TElement&>())),
        decltype(std::declval<TQueue&>().ReceiveFromInterrupt(std::declval<TElement&>()))>>
    : std::bool_constant<
          std::is_same_v<decltype(std::declval<TQueue&>().SendFromInterrupt(
                             std::declval<const TElement&>())),
                         InterruptOperationResult> &&
          std::is_same_v<decltype(std::declval<TQueue&>().ReceiveFromInterrupt(
                             std::declval<TElement&>())),
                         InterruptOperationResult> &&
          noexcept(std::declval<TQueue&>().SendFromInterrupt(std::declval<const TElement&>())) &&
          noexcept(std::declval<TQueue&>().ReceiveFromInterrupt(std::declval<TElement&>()))> {};

template <typename TQueue, typename = void>
struct IsStaticQueue : std::false_type {};

template <typename TQueue>
struct IsStaticQueue<
    TQueue,
    std::void_t<
        decltype(TQueue::ControlStorageBytes),
        decltype(TQueue::TotalStaticStorageBytes)>>
    : std::bool_constant<
          static_cast<std::size_t>(TQueue::TotalStaticStorageBytes) >=
              static_cast<std::size_t>(TQueue::PayloadStorageBytes) +
              static_cast<std::size_t>(TQueue::ControlStorageBytes)> {};

template <typename TExtraCapabilities>
struct DeclarationBuilder;

template <typename... TExtraCapabilities>
struct DeclarationBuilder<CapabilitySet<TExtraCapabilities...>> {
    template <typename TOrigin, typename TRequirements>
    using Type = ESPressio::Platform::ProviderDeclaration<
        TOrigin,
        CapabilitySet<Capability::Queue, TExtraCapabilities...>,
        TRequirements>;
};

template <typename T, typename = void>
struct IsQueueProvider : std::false_type {};

template <typename T>
struct IsQueueProvider<
    T,
    std::void_t<
        typename T::PlatformCapabilities,
        typename T::template Queue<std::uint32_t, 2U>>>
    : std::bool_constant<
          ESPressio::Platform::IsProviderV<T> &&
          T::PlatformCapabilities::template Contains<Capability::Queue> &&
          IsQueue<typename T::template Queue<std::uint32_t, 2U>, std::uint32_t, 2U>::value &&
          (!T::PlatformCapabilities::template Contains<Capability::StaticQueueStorage> ||
           IsStaticQueue<typename T::template Queue<std::uint32_t, 2U>>::value) &&
          (!(T::PlatformCapabilities::template Contains<Capability::QueueSendFromInterrupt> ||
             T::PlatformCapabilities::template Contains<Capability::QueueReceiveFromInterrupt>) ||
           IsInterruptQueue<typename T::template Queue<std::uint32_t, 2U>, std::uint32_t>::value)> {};

} // namespace Detail

template <typename TOrigin,
          typename TExtraCapabilities = CapabilitySet<>,
          typename TRequirements = RequirementSet<>>
struct ProviderDeclaration
    : Detail::DeclarationBuilder<TExtraCapabilities>::template Type<
          TOrigin, TRequirements> {};

template <typename TQueue, typename TElement, std::size_t TDepth>
inline constexpr bool IsQueueV = Detail::IsQueue<TQueue, TElement, TDepth>::value;

template <typename TQueue, typename TElement>
inline constexpr bool IsInterruptQueueV = Detail::IsInterruptQueue<TQueue, TElement>::value;

template <typename TQueue>
inline constexpr bool IsStaticQueueV = Detail::IsStaticQueue<TQueue>::value;

template <typename T>
inline constexpr bool IsProviderV = Detail::IsQueueProvider<T>::value;

} // namespace Queue

} // namespace ESPressio::Platform
