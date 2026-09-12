#pragma once

#include <cstdint>
#include <type_traits>
#include <utility>

#include "ESPressio_PlatformCapabilities.hpp"
#include "ESPressio_PlatformIO.hpp"

namespace ESPressio::Platform {

namespace GPIO {

struct DefaultDomain {};

using Pin = std::uint32_t;

enum class Level : std::uint8_t { Low = 0, High = 1 };
enum class Direction : std::uint8_t { Input = 0, Output, InputOutput };
enum class Pull : std::uint8_t { None = 0, Up, Down, UpDown };
enum class OutputMode : std::uint8_t { PushPull = 0, OpenDrain };
enum class InterruptTrigger : std::uint8_t {
    RisingEdge = 0,
    FallingEdge,
    AnyEdge,
    LowLevel,
    HighLevel
};

struct Configuration {
    Direction DirectionMode{Direction::Input};
    Pull PullMode{Pull::None};
    OutputMode Output{OutputMode::PushPull};
    Level InitialLevel{Level::Low};
    bool ApplyInitialLevel{false};
};

using InterruptHandler = void (*)(void*) noexcept;

} // namespace GPIO

namespace Capability {

template <typename TDomainTag>
struct GPIODomain final : ExclusiveCapability {};

template <typename TDomainTag>
struct GPIOPullUp final : SharedCapability {};

template <typename TDomainTag>
struct GPIOPullDown final : SharedCapability {};

template <typename TDomainTag>
struct GPIOOpenDrain final : SharedCapability {};

template <typename TDomainTag>
struct GPIOBidirectional final : SharedCapability {};

template <typename TDomainTag>
struct GPIOInterrupts final : SharedCapability {};

} // namespace Capability

namespace GPIO {

template <typename TDomainTag>
using CapabilityFor = std::conditional_t<
    std::is_same_v<TDomainTag, DefaultDomain>,
    ESPressio::Platform::Capability::GPIO,
    ESPressio::Platform::Capability::GPIODomain<TDomainTag>>;

namespace Detail {

template <typename TExtraCapabilities>
struct DeclarationBuilder;

template <typename... TExtraCapabilities>
struct DeclarationBuilder<CapabilitySet<TExtraCapabilities...>> {
    template <typename TOrigin, typename TDomainTag, typename TRequirements>
    using Type = ESPressio::Platform::ProviderDeclaration<
        TOrigin,
        CapabilitySet<CapabilityFor<TDomainTag>, TExtraCapabilities...>,
        TRequirements>;
};

template <typename T, typename TDomainTag, typename = void>
struct IsController : std::false_type {};

template <typename T, typename TDomainTag>
struct IsController<
    T,
    TDomainTag,
    std::void_t<
        typename T::PlatformCapabilities,
        decltype(std::declval<T&>().Configure(Pin{}, std::declval<const Configuration&>())),
        decltype(std::declval<T&>().Read(Pin{}, std::declval<Level&>())),
        decltype(std::declval<T&>().Write(Pin{}, Level::Low))>>
    : std::bool_constant<
          IsProviderV<T> &&
          T::PlatformCapabilities::template Contains<CapabilityFor<TDomainTag>> &&
          std::is_same_v<decltype(std::declval<T&>().Configure(
                             Pin{}, std::declval<const Configuration&>())),
                         IO::Result> &&
          std::is_same_v<decltype(std::declval<T&>().Read(Pin{}, std::declval<Level&>())),
                         IO::Result> &&
          std::is_same_v<decltype(std::declval<T&>().Write(Pin{}, Level::Low)),
                         IO::Result> &&
          noexcept(std::declval<T&>().Configure(Pin{}, std::declval<const Configuration&>())) &&
          noexcept(std::declval<T&>().Read(Pin{}, std::declval<Level&>())) &&
          noexcept(std::declval<T&>().Write(Pin{}, Level::Low))> {};

template <typename T, typename TDomainTag, typename = void>
struct IsInterruptController : std::false_type {};

template <typename T, typename TDomainTag>
struct IsInterruptController<
    T,
    TDomainTag,
    std::void_t<
        decltype(std::declval<T&>().InitializeInterrupts()),
        decltype(std::declval<T&>().AttachInterrupt(
            Pin{}, InterruptTrigger::RisingEdge,
            static_cast<InterruptHandler>(nullptr), static_cast<void*>(nullptr))),
        decltype(std::declval<T&>().DetachInterrupt(Pin{}))>>
    : std::bool_constant<
          IsController<T, TDomainTag>::value &&
          T::PlatformCapabilities::template Contains<Capability::GPIOInterrupts<TDomainTag>> &&
          std::is_same_v<decltype(std::declval<T&>().InitializeInterrupts()), IO::Result> &&
          std::is_same_v<decltype(std::declval<T&>().AttachInterrupt(
                             Pin{}, InterruptTrigger::RisingEdge,
                             static_cast<InterruptHandler>(nullptr), static_cast<void*>(nullptr))),
                         IO::Result> &&
          std::is_same_v<decltype(std::declval<T&>().DetachInterrupt(Pin{})), IO::Result> &&
          noexcept(std::declval<T&>().InitializeInterrupts()) &&
          noexcept(std::declval<T&>().AttachInterrupt(
              Pin{}, InterruptTrigger::RisingEdge,
              static_cast<InterruptHandler>(nullptr), static_cast<void*>(nullptr))) &&
          noexcept(std::declval<T&>().DetachInterrupt(Pin{}))> {};

} // namespace Detail

template <typename TOrigin,
          typename TDomainTag = DefaultDomain,
          typename TExtraCapabilities = CapabilitySet<>,
          typename TRequirements = RequirementSet<>>
struct ProviderDeclaration
    : Detail::DeclarationBuilder<TExtraCapabilities>::template Type<
          TOrigin, TDomainTag, TRequirements> {};

template <typename T, typename TDomainTag = DefaultDomain>
inline constexpr bool IsControllerV = Detail::IsController<T, TDomainTag>::value;

template <typename T, typename TDomainTag = DefaultDomain>
inline constexpr bool IsInterruptControllerV =
    Detail::IsInterruptController<T, TDomainTag>::value;

} // namespace GPIO

} // namespace ESPressio::Platform
