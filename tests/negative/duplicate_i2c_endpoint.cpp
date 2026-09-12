#include "ESPressio_Platform.hpp"
using namespace ESPressio::Platform;
struct O final : Backend {};
struct Channel {};
struct A : I2C::DeviceProviderDeclaration<O,Channel,0x10,I2C::AddressWidth::Bits7,100000> {
 IO::Result Write(IO::ConstBuffer) noexcept{return IO::Result::Ok;} IO::Result Read(IO::MutableBuffer) noexcept{return IO::Result::Ok;} IO::Result WriteRead(IO::ConstBuffer,IO::MutableBuffer) noexcept{return IO::Result::Ok;}
};
struct B : I2C::DeviceProviderDeclaration<O,Channel,0x11,I2C::AddressWidth::Bits7,100000> {
 IO::Result Write(IO::ConstBuffer) noexcept{return IO::Result::Ok;} IO::Result Read(IO::MutableBuffer) noexcept{return IO::Result::Ok;} IO::Result WriteRead(IO::ConstBuffer,IO::MutableBuffer) noexcept{return IO::Result::Ok;}
};
using Invalid = Composition<A,B>;
static_assert(Invalid::IsValid);
int main(){}
