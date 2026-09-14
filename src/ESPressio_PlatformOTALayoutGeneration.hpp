#pragma once

#include <cstdint>

namespace ESPressio::Platform::OTA {

/// <summary>Monotonic semantic generation of one Platform storage-layout identity. Zero is invalid/unspecified.</summary>
class StorageLayoutGeneration final {
    std::uint32_t value_{};
public:
    constexpr StorageLayoutGeneration() noexcept = default;
    constexpr explicit StorageLayoutGeneration(std::uint32_t value) noexcept : value_(value) {}
    constexpr std::uint32_t Value() const noexcept { return value_; }
    constexpr explicit operator bool() const noexcept { return value_ != 0U; }
    constexpr bool operator==(StorageLayoutGeneration other) const noexcept { return value_ == other.value_; }
    constexpr bool operator!=(StorageLayoutGeneration other) const noexcept { return !(*this == other); }
    constexpr bool operator<(StorageLayoutGeneration other) const noexcept { return value_ < other.value_; }
    constexpr bool operator<=(StorageLayoutGeneration other) const noexcept { return value_ <= other.value_; }
    constexpr bool operator>(StorageLayoutGeneration other) const noexcept { return value_ > other.value_; }
    constexpr bool operator>=(StorageLayoutGeneration other) const noexcept { return value_ >= other.value_; }
};

static_assert(sizeof(StorageLayoutGeneration) == 4U, "StorageLayoutGeneration must be exactly four bytes");

} // namespace ESPressio::Platform::OTA
