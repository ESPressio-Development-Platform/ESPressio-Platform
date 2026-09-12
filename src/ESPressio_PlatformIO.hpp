#pragma once

#include <cstddef>
#include <cstdint>

namespace ESPressio::Platform::IO {

/** Common result vocabulary for deterministic platform I/O operations. */
enum class Result : std::uint8_t {
    Ok = 0,
    InvalidArgument,
    NotInitialized,
    Unsupported,
    Busy,
    Timeout,
    NoDevice,
    ResourceExhausted,
    IoError
};

/** Immutable byte range. A null pointer is valid only when Size is zero. */
struct ConstBuffer {
    const std::uint8_t* Data{nullptr};
    std::size_t Size{0U};

    constexpr bool IsValid() const noexcept { return Data != nullptr || Size == 0U; }
    constexpr bool Empty() const noexcept { return Size == 0U; }
};

/** Mutable byte range. A null pointer is valid only when Size is zero. */
struct MutableBuffer {
    std::uint8_t* Data{nullptr};
    std::size_t Size{0U};

    constexpr bool IsValid() const noexcept { return Data != nullptr || Size == 0U; }
    constexpr bool Empty() const noexcept { return Size == 0U; }
    constexpr operator ConstBuffer() const noexcept { return {Data, Size}; }
};

/** Result of an operation that may transfer fewer bytes than requested. */
struct TransferResult {
    Result Status{Result::Ok};
    std::size_t Count{0U};

    constexpr bool Succeeded() const noexcept { return Status == Result::Ok; }
    constexpr bool Completed(std::size_t requested) const noexcept {
        return Status == Result::Ok && Count == requested;
    }
};

constexpr ConstBuffer Bytes(const std::uint8_t* data, std::size_t size) noexcept {
    return {data, size};
}

constexpr MutableBuffer Bytes(std::uint8_t* data, std::size_t size) noexcept {
    return {data, size};
}

template <std::size_t N>
constexpr ConstBuffer Bytes(const std::uint8_t (&data)[N]) noexcept {
    return {data, N};
}

template <std::size_t N>
constexpr MutableBuffer Bytes(std::uint8_t (&data)[N]) noexcept {
    return {data, N};
}

} // namespace ESPressio::Platform::IO
