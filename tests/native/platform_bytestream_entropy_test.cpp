#include <cstddef>
#include <cstdint>
#include <type_traits>

#include "ESPressio_Platform.hpp"

using namespace ESPressio::Platform;

namespace {

struct TestOrigin final : Backend {};
struct AuxiliaryStreamTag {};

struct ConsoleStream final : ByteStream::ProviderDeclaration<TestOrigin> {
    IO::Result Available(std::size_t& count) noexcept {
        count = 0U;
        return IO::Result::Ok;
    }

    IO::TransferResult Read(IO::MutableBuffer buffer) noexcept {
        if (!buffer.IsValid()) return {IO::Result::InvalidArgument, 0U};
        return {IO::Result::Ok, 0U};
    }

    IO::TransferResult Write(IO::ConstBuffer buffer) noexcept {
        if (!buffer.IsValid()) return {IO::Result::InvalidArgument, 0U};
        return {IO::Result::Ok, buffer.Size};
    }
};

struct AuxiliaryStream final
    : ByteStream::ProviderDeclaration<TestOrigin, AuxiliaryStreamTag> {
    IO::Result Available(std::size_t& count) noexcept {
        count = 0U;
        return IO::Result::Ok;
    }

    IO::TransferResult Read(IO::MutableBuffer buffer) noexcept {
        if (!buffer.IsValid()) return {IO::Result::InvalidArgument, 0U};
        return {IO::Result::Ok, 0U};
    }

    IO::TransferResult Write(IO::ConstBuffer buffer) noexcept {
        if (!buffer.IsValid()) return {IO::Result::InvalidArgument, 0U};
        return {IO::Result::Ok, buffer.Size};
    }
};

struct EntropySource final
    : Entropy::ProviderDeclaration<
          TestOrigin,
          CapabilitySet<Capability::HardwareRandomGenerator>> {
    IO::Result Fill(IO::MutableBuffer buffer) noexcept {
        if (!buffer.IsValid()) return IO::Result::InvalidArgument;
        for (std::size_t i = 0U; i < buffer.Size; ++i)
            buffer.Data[i] = static_cast<std::uint8_t>(i * 37U + 11U);
        return IO::Result::Ok;
    }
};

struct InvalidStreamShape final
    : ByteStream::ProviderDeclaration<TestOrigin, struct InvalidStreamTag> {
    int Available(std::size_t&) noexcept { return 0; }
};

static_assert(ByteStream::IsStreamV<ConsoleStream>);
static_assert(ByteStream::IsStreamV<AuxiliaryStream, AuxiliaryStreamTag>);
static_assert(!ByteStream::IsStreamV<InvalidStreamShape, InvalidStreamTag>);
static_assert(Entropy::IsSourceV<EntropySource>);

using TestPlatform = Composition<ConsoleStream, AuxiliaryStream, EntropySource>;
static_assert(TestPlatform::IsValid);
static_assert(TestPlatform::Provides<Capability::ByteStream>);
static_assert(TestPlatform::Provides<Capability::ByteStreamEndpoint<AuxiliaryStreamTag>>);
static_assert(TestPlatform::Provides<Capability::Entropy>);
static_assert(TestPlatform::Provides<Capability::HardwareRandomGenerator>);
static_assert(std::is_same_v<TestPlatform::ProviderFor<Capability::ByteStream>, ConsoleStream>);
static_assert(std::is_same_v<
    TestPlatform::ProviderFor<Capability::ByteStreamEndpoint<AuxiliaryStreamTag>>,
    AuxiliaryStream>);

} // namespace

int main() {
    ConsoleStream stream;
    std::uint8_t bytes[4]{};
    const auto write = stream.Write(IO::Bytes(bytes));
    if (!write.Completed(sizeof(bytes))) return 1;

    EntropySource entropy;
    if (entropy.Fill(IO::Bytes(bytes)) != IO::Result::Ok) return 2;
    return bytes[0] == 11U ? 0 : 3;
}
