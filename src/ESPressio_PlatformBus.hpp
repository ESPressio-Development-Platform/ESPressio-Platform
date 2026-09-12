#pragma once

#include <cstddef>
#include <cstdint>
#include <type_traits>
#include <utility>

#include "ESPressio_PlatformCapabilities.hpp"
#include "ESPressio_PlatformIO.hpp"

namespace ESPressio::Platform {

namespace I2C {

enum class AddressWidth : std::uint8_t {
    Bits7 = 7,
    Bits10 = 10
};

} // namespace I2C

namespace SPI {

enum class Mode : std::uint8_t {
    Mode0 = 0,
    Mode1 = 1,
    Mode2 = 2,
    Mode3 = 3
};

enum class BitOrder : std::uint8_t {
    MostSignificantFirst = 0,
    LeastSignificantFirst = 1
};

} // namespace SPI

namespace Capability {

template <typename TDeviceTag>
struct I2CDevice final : ExclusiveCapability {};

template <typename TDeviceTag>
struct I2CRepeatedStart final : SharedCapability {};

template <typename TDeviceTag>
struct SPIDevice final : ExclusiveCapability {};

template <typename TDeviceTag>
struct SPIFullDuplex final : SharedCapability {};

} // namespace Capability

namespace PropertyKey {

struct BusFrequencyHz final : Property<std::uint32_t> {};
struct I2CAddress final : Property<std::uint16_t> {};
struct I2CAddressBits final : Property<std::uint8_t> {};
struct SPIMode final : Property<std::uint8_t> {};
struct SPIBitOrder final : Property<std::uint8_t> {};

} // namespace PropertyKey

namespace I2C {

namespace Detail {

template <typename TExtraCapabilities>
struct DeclarationBuilder;

template <typename... TExtraCapabilities>
struct DeclarationBuilder<CapabilitySet<TExtraCapabilities...>> {
    template <typename TOrigin,
              typename TDeviceTag,
              std::uint16_t TAddress,
              AddressWidth TAddressWidth,
              std::uint32_t TFrequencyHz,
              typename TRequirements>
    using Type = ESPressio::Platform::ProviderDeclaration<
        TOrigin,
        CapabilitySet<
            CapabilityProfile<
                Capability::I2CDevice<TDeviceTag>,
                PropertyValue<PropertyKey::I2CAddress, TAddress>,
                PropertyValue<PropertyKey::I2CAddressBits,
                              static_cast<std::uint8_t>(TAddressWidth)>,
                PropertyValue<PropertyKey::BusFrequencyHz, TFrequencyHz>>,
            Capability::I2CRepeatedStart<TDeviceTag>,
            TExtraCapabilities...>,
        TRequirements>;
};

template <typename T, typename TDeviceTag, typename = void>
struct IsDevice : std::false_type {};

template <typename T, typename TDeviceTag>
struct IsDevice<
    T,
    TDeviceTag,
    std::void_t<
        typename T::PlatformCapabilities,
        decltype(std::declval<T&>().Write(std::declval<IO::ConstBuffer>())),
        decltype(std::declval<T&>().Read(std::declval<IO::MutableBuffer>())),
        decltype(std::declval<T&>().WriteRead(std::declval<IO::ConstBuffer>(),
                                              std::declval<IO::MutableBuffer>()))>>
    : std::bool_constant<
          IsProviderV<T> &&
          T::PlatformCapabilities::template Contains<Capability::I2CDevice<TDeviceTag>> &&
          T::PlatformCapabilities::template Contains<Capability::I2CRepeatedStart<TDeviceTag>> &&
          std::is_same_v<decltype(std::declval<T&>().Write(std::declval<IO::ConstBuffer>())),
                         IO::Result> &&
          std::is_same_v<decltype(std::declval<T&>().Read(std::declval<IO::MutableBuffer>())),
                         IO::Result> &&
          std::is_same_v<decltype(std::declval<T&>().WriteRead(std::declval<IO::ConstBuffer>(),
                                                               std::declval<IO::MutableBuffer>())),
                         IO::Result> &&
          noexcept(std::declval<T&>().Write(std::declval<IO::ConstBuffer>())) &&
          noexcept(std::declval<T&>().Read(std::declval<IO::MutableBuffer>())) &&
          noexcept(std::declval<T&>().WriteRead(std::declval<IO::ConstBuffer>(),
                                                std::declval<IO::MutableBuffer>()))> {};

} // namespace Detail

template <typename TOrigin,
          typename TDeviceTag,
          std::uint16_t TAddress,
          AddressWidth TAddressWidth,
          std::uint32_t TFrequencyHz,
          typename TExtraCapabilities = CapabilitySet<>,
          typename TRequirements = RequirementSet<>>
struct DeviceProviderDeclaration
    : Detail::DeclarationBuilder<TExtraCapabilities>::template Type<
          TOrigin,
          TDeviceTag,
          TAddress,
          TAddressWidth,
          TFrequencyHz,
          TRequirements> {
    static_assert(TFrequencyHz > 0U, "I2C endpoint frequency must be greater than zero");
    static_assert(TAddressWidth == AddressWidth::Bits7 || TAddressWidth == AddressWidth::Bits10,
                  "I2C endpoint address width must be 7 or 10 bits");
    static_assert(TAddressWidth != AddressWidth::Bits7 || TAddress <= 0x7FU,
                  "7-bit I2C address is out of range");
    static_assert(TAddressWidth != AddressWidth::Bits10 || TAddress <= 0x3FFU,
                  "10-bit I2C address is out of range");
};

template <typename T, typename TDeviceTag>
inline constexpr bool IsDeviceV = Detail::IsDevice<T, TDeviceTag>::value;

template <typename TDeviceTag, std::uint32_t TMinimumFrequencyHz>
using MinimumFrequency = CapabilityRequirement<
    Capability::I2CDevice<TDeviceTag>,
    PropertyAtLeast<PropertyKey::BusFrequencyHz, TMinimumFrequencyHz>>;

} // namespace I2C

namespace SPI {

namespace Detail {

template <typename TExtraCapabilities>
struct DeclarationBuilder;

template <typename... TExtraCapabilities>
struct DeclarationBuilder<CapabilitySet<TExtraCapabilities...>> {
    template <typename TOrigin,
              typename TDeviceTag,
              std::uint32_t TFrequencyHz,
              Mode TMode,
              BitOrder TBitOrder,
              typename TRequirements>
    using Type = ESPressio::Platform::ProviderDeclaration<
        TOrigin,
        CapabilitySet<
            CapabilityProfile<
                Capability::SPIDevice<TDeviceTag>,
                PropertyValue<PropertyKey::BusFrequencyHz, TFrequencyHz>,
                PropertyValue<PropertyKey::SPIMode, static_cast<std::uint8_t>(TMode)>,
                PropertyValue<PropertyKey::SPIBitOrder, static_cast<std::uint8_t>(TBitOrder)>>,
            Capability::SPIFullDuplex<TDeviceTag>,
            TExtraCapabilities...>,
        TRequirements>;
};

template <typename T, typename TDeviceTag, typename = void>
struct IsDevice : std::false_type {};

template <typename T, typename TDeviceTag>
struct IsDevice<
    T,
    TDeviceTag,
    std::void_t<
        typename T::PlatformCapabilities,
        decltype(std::declval<T&>().Transfer(
            static_cast<const std::uint8_t*>(nullptr),
            static_cast<std::uint8_t*>(nullptr),
            std::size_t{0}))>>
    : std::bool_constant<
          IsProviderV<T> &&
          T::PlatformCapabilities::template Contains<Capability::SPIDevice<TDeviceTag>> &&
          T::PlatformCapabilities::template Contains<Capability::SPIFullDuplex<TDeviceTag>> &&
          std::is_same_v<decltype(std::declval<T&>().Transfer(
                             static_cast<const std::uint8_t*>(nullptr),
                             static_cast<std::uint8_t*>(nullptr),
                             std::size_t{0})),
                         IO::Result> &&
          noexcept(std::declval<T&>().Transfer(
              static_cast<const std::uint8_t*>(nullptr),
              static_cast<std::uint8_t*>(nullptr),
              std::size_t{0}))> {};

} // namespace Detail

template <typename TOrigin,
          typename TDeviceTag,
          std::uint32_t TFrequencyHz,
          Mode TMode,
          BitOrder TBitOrder,
          typename TExtraCapabilities = CapabilitySet<>,
          typename TRequirements = RequirementSet<>>
struct DeviceProviderDeclaration
    : Detail::DeclarationBuilder<TExtraCapabilities>::template Type<
          TOrigin,
          TDeviceTag,
          TFrequencyHz,
          TMode,
          TBitOrder,
          TRequirements> {
    static_assert(TFrequencyHz > 0U, "SPI endpoint frequency must be greater than zero");
};

template <typename T, typename TDeviceTag>
inline constexpr bool IsDeviceV = Detail::IsDevice<T, TDeviceTag>::value;

template <typename TDeviceTag, std::uint32_t TMinimumFrequencyHz>
using MinimumFrequency = CapabilityRequirement<
    Capability::SPIDevice<TDeviceTag>,
    PropertyAtLeast<PropertyKey::BusFrequencyHz, TMinimumFrequencyHz>>;

} // namespace SPI

} // namespace ESPressio::Platform
