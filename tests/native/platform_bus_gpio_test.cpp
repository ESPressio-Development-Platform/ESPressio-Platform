#include <type_traits>

#include "ESPressio_Platform.hpp"

using namespace ESPressio::Platform;

namespace {

struct TestOrigin final : Backend {};
struct RTCChannel {};
struct SensorChannel {};
struct ExpansionGPIO {};

struct RTCI2C final
    : I2C::DeviceProviderDeclaration<TestOrigin,
                                     RTCChannel,
                                     0x68U,
                                     I2C::AddressWidth::Bits7,
                                     400'000U> {
    IO::Result Write(IO::ConstBuffer) noexcept { return IO::Result::Ok; }
    IO::Result Read(IO::MutableBuffer) noexcept { return IO::Result::Ok; }
    IO::Result WriteRead(IO::ConstBuffer, IO::MutableBuffer) noexcept { return IO::Result::Ok; }
};

struct SensorSPI final
    : SPI::DeviceProviderDeclaration<TestOrigin,
                                     SensorChannel,
                                     8'000'000U,
                                     SPI::Mode::Mode3,
                                     SPI::BitOrder::MostSignificantFirst> {
    IO::Result Transfer(const std::uint8_t*, std::uint8_t*, std::size_t) noexcept {
        return IO::Result::Ok;
    }
};

struct NativeGPIO final
    : GPIO::ProviderDeclaration<
          TestOrigin,
          GPIO::DefaultDomain,
          CapabilitySet<Capability::GPIOPullUp<GPIO::DefaultDomain>>> {
    IO::Result Configure(GPIO::Pin, const GPIO::Configuration&) noexcept { return IO::Result::Ok; }
    IO::Result Read(GPIO::Pin, GPIO::Level&) noexcept { return IO::Result::Ok; }
    IO::Result Write(GPIO::Pin, GPIO::Level) noexcept { return IO::Result::Ok; }
};

struct ExpansionGPIOProvider final
    : GPIO::ProviderDeclaration<TestOrigin, ExpansionGPIO> {
    IO::Result Configure(GPIO::Pin, const GPIO::Configuration&) noexcept { return IO::Result::Ok; }
    IO::Result Read(GPIO::Pin, GPIO::Level&) noexcept { return IO::Result::Ok; }
    IO::Result Write(GPIO::Pin, GPIO::Level) noexcept { return IO::Result::Ok; }
};

using PlatformUnderTest = Composition<RTCI2C, SensorSPI, NativeGPIO, ExpansionGPIOProvider>;

static_assert(I2C::IsDeviceV<RTCI2C, RTCChannel>);
static_assert(SPI::IsDeviceV<SensorSPI, SensorChannel>);
static_assert(GPIO::IsControllerV<NativeGPIO>);
static_assert(GPIO::IsControllerV<ExpansionGPIOProvider, ExpansionGPIO>);

static_assert(PlatformUnderTest::Provides<Capability::I2CDevice<RTCChannel>>);
static_assert(PlatformUnderTest::Provides<Capability::SPIDevice<SensorChannel>>);
static_assert(PlatformUnderTest::Provides<Capability::GPIO>);
static_assert(PlatformUnderTest::Provides<Capability::GPIODomain<ExpansionGPIO>>);

static_assert(PlatformUnderTest::PropertyValue<Capability::I2CDevice<RTCChannel>,
                                               PropertyKey::I2CAddress> == 0x68U);
static_assert(PlatformUnderTest::PropertyValue<Capability::I2CDevice<RTCChannel>,
                                               PropertyKey::BusFrequencyHz> == 400'000U);
static_assert(PlatformUnderTest::PropertyValue<Capability::SPIDevice<SensorChannel>,
                                               PropertyKey::SPIMode> == 3U);

using FastEnoughRTC = I2C::MinimumFrequency<RTCChannel, 100'000U>;
using TooFastRTC = I2C::MinimumFrequency<RTCChannel, 1'000'000U>;
static_assert(PlatformUnderTest::Satisfies<RequirementSet<FastEnoughRTC>>);
static_assert(!PlatformUnderTest::Satisfies<RequirementSet<TooFastRTC>>);

static_assert(std::is_convertible_v<IO::MutableBuffer, IO::ConstBuffer>);

} // namespace

int main() { return 0; }
