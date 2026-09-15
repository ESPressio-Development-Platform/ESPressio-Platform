#pragma once

#include <cstddef>
#include <cstdint>
#include <type_traits>
#include <utility>

#include "ESPressio_PlatformCapabilities.hpp"

namespace ESPressio::Platform {

namespace Capability {
struct ApplicationImageStaging final : ExclusiveCapability {};
struct FilesystemImageStaging final : ExclusiveCapability {};
struct BootControl final : ExclusiveCapability {};
struct TrialBoot final : ExclusiveCapability {};
struct SystemRestart final : ExclusiveCapability {};
struct StorageLayoutInspection final : ExclusiveCapability {};
struct StorageLayoutTransition final : ExclusiveCapability {};
} // namespace Capability

namespace OTA {

class BootTargetIdentifier final {
    std::uint32_t value_{};
public:
    constexpr BootTargetIdentifier() noexcept = default;
    constexpr explicit BootTargetIdentifier(std::uint32_t value) noexcept : value_(value) {}
    constexpr std::uint32_t Value() const noexcept { return value_; }
    constexpr explicit operator bool() const noexcept { return value_ != 0U; }
    constexpr bool operator==(BootTargetIdentifier other) const noexcept { return value_ == other.value_; }
    constexpr bool operator!=(BootTargetIdentifier other) const noexcept { return !(*this == other); }
    constexpr bool operator<(BootTargetIdentifier other) const noexcept { return value_ < other.value_; }
};
static_assert(sizeof(BootTargetIdentifier) == 4U, "BootTargetIdentifier must be exactly four bytes");

class StorageLayoutIdentifier final {
    std::uint64_t value_{};
public:
    constexpr StorageLayoutIdentifier() noexcept = default;
    constexpr explicit StorageLayoutIdentifier(std::uint64_t value) noexcept : value_(value) {}
    constexpr std::uint64_t Value() const noexcept { return value_; }
    constexpr explicit operator bool() const noexcept { return value_ != 0U; }
    constexpr bool operator==(StorageLayoutIdentifier other) const noexcept { return value_ == other.value_; }
    constexpr bool operator!=(StorageLayoutIdentifier other) const noexcept { return !(*this == other); }
    constexpr bool operator<(StorageLayoutIdentifier other) const noexcept { return value_ < other.value_; }
};
static_assert(sizeof(StorageLayoutIdentifier) == 8U, "StorageLayoutIdentifier must be exactly eight bytes");

enum class RestartReason : std::uint8_t {
    Unspecified,
    ActivateCandidate,
    Rollback,
    CompleteStorageLayoutTransition,
    Recovery
};

enum class Status : std::uint8_t {
    Success,
    Pending,
    Unsupported,
    Invalid,
    Busy,
    CapacityUnavailable,
    Failed
};

struct Result final {
    Status Code{Status::Failed};
    std::int32_t NativeCode{0};
    constexpr explicit operator bool() const noexcept { return Code == Status::Success; }
};

enum class TrialBootState : std::uint8_t {
    Unknown,
    Untracked,
    NeverAttempted,
    Armed,
    PendingValidation,
    Accepted,
    Rejected
};

struct TrialBootStateResult final {
    Status Code{Status::Failed};
    TrialBootState State{TrialBootState::Unknown};
    std::int32_t NativeCode{0};
    constexpr explicit operator bool() const noexcept { return Code == Status::Success; }
};

struct PreflightResult final {
    Status Code{Status::Failed};
    std::uint64_t RequiredBytes{0};
    std::uint64_t MaximumBytes{0};
    std::int32_t NativeCode{0};
    constexpr explicit operator bool() const noexcept { return Code == Status::Success; }
};

struct WriteResult final {
    Status Code{Status::Failed};
    std::size_t ConsumedBytes{0};
    std::int32_t NativeCode{0};
    constexpr explicit operator bool() const noexcept { return Code == Status::Success; }
};

struct StorageLayoutInfo final {
    StorageLayoutIdentifier Layout{};
    std::uint64_t TotalBytes{0};
    std::uint64_t AvailableBytes{0};
};

namespace Detail {

template <typename T, typename = void>
struct IsApplicationImageStagingProvider : std::false_type {};

template <typename T>
struct IsApplicationImageStagingProvider<T, std::void_t<
    typename T::PlatformCapabilities,
    decltype(std::declval<T&>().PreflightApplicationImage(std::uint64_t{})),
    decltype(std::declval<T&>().BeginApplicationImage(std::uint64_t{})),
    decltype(std::declval<T&>().WriteApplicationImage(static_cast<const std::uint8_t*>(nullptr), std::size_t{})),
    decltype(std::declval<T&>().FinalizeApplicationImage()),
    decltype(std::declval<T&>().AbortApplicationImage()),
    decltype(std::declval<const T&>().ApplicationImageTarget())>>
    : std::bool_constant<
          IsProviderV<T> &&
          T::PlatformCapabilities::template Contains<Capability::ApplicationImageStaging> &&
          std::is_same_v<decltype(std::declval<T&>().PreflightApplicationImage(std::uint64_t{})), PreflightResult> &&
          std::is_same_v<decltype(std::declval<T&>().BeginApplicationImage(std::uint64_t{})), Result> &&
          std::is_same_v<decltype(std::declval<T&>().WriteApplicationImage(static_cast<const std::uint8_t*>(nullptr), std::size_t{})), WriteResult> &&
          std::is_same_v<decltype(std::declval<T&>().FinalizeApplicationImage()), Result> &&
          std::is_same_v<decltype(std::declval<T&>().AbortApplicationImage()), Result> &&
          std::is_same_v<decltype(std::declval<const T&>().ApplicationImageTarget()), BootTargetIdentifier> &&
          noexcept(std::declval<T&>().PreflightApplicationImage(std::uint64_t{})) &&
          noexcept(std::declval<T&>().BeginApplicationImage(std::uint64_t{})) &&
          noexcept(std::declval<T&>().WriteApplicationImage(static_cast<const std::uint8_t*>(nullptr), std::size_t{})) &&
          noexcept(std::declval<T&>().FinalizeApplicationImage()) &&
          noexcept(std::declval<T&>().AbortApplicationImage()) &&
          noexcept(std::declval<const T&>().ApplicationImageTarget())> {};

template <typename T, typename = void>
struct IsFilesystemImageStagingProvider : std::false_type {};

template <typename T>
struct IsFilesystemImageStagingProvider<T, std::void_t<
    typename T::PlatformCapabilities,
    decltype(std::declval<T&>().PreflightFilesystemImage(std::uint64_t{})),
    decltype(std::declval<T&>().BeginFilesystemImage(std::uint64_t{})),
    decltype(std::declval<T&>().WriteFilesystemImage(static_cast<const std::uint8_t*>(nullptr), std::size_t{})),
    decltype(std::declval<T&>().FinalizeFilesystemImage()),
    decltype(std::declval<T&>().AbortFilesystemImage())>>
    : std::bool_constant<
          IsProviderV<T> &&
          T::PlatformCapabilities::template Contains<Capability::FilesystemImageStaging> &&
          std::is_same_v<decltype(std::declval<T&>().PreflightFilesystemImage(std::uint64_t{})), PreflightResult> &&
          std::is_same_v<decltype(std::declval<T&>().BeginFilesystemImage(std::uint64_t{})), Result> &&
          std::is_same_v<decltype(std::declval<T&>().WriteFilesystemImage(static_cast<const std::uint8_t*>(nullptr), std::size_t{})), WriteResult> &&
          std::is_same_v<decltype(std::declval<T&>().FinalizeFilesystemImage()), Result> &&
          std::is_same_v<decltype(std::declval<T&>().AbortFilesystemImage()), Result> &&
          noexcept(std::declval<T&>().PreflightFilesystemImage(std::uint64_t{})) &&
          noexcept(std::declval<T&>().BeginFilesystemImage(std::uint64_t{})) &&
          noexcept(std::declval<T&>().WriteFilesystemImage(static_cast<const std::uint8_t*>(nullptr), std::size_t{})) &&
          noexcept(std::declval<T&>().FinalizeFilesystemImage()) &&
          noexcept(std::declval<T&>().AbortFilesystemImage())> {};

template <typename T, typename = void>
struct IsBootControlProvider : std::false_type {};

template <typename T>
struct IsBootControlProvider<T, std::void_t<
    typename T::PlatformCapabilities,
    decltype(std::declval<const T&>().CurrentBootTarget()),
    decltype(std::declval<const T&>().CommittedBootTarget()),
    decltype(std::declval<const T&>().NextBootTarget()),
    decltype(std::declval<T&>().SelectNextBootTarget(BootTargetIdentifier{}))>>
    : std::bool_constant<
          IsProviderV<T> &&
          T::PlatformCapabilities::template Contains<Capability::BootControl> &&
          std::is_same_v<decltype(std::declval<const T&>().CurrentBootTarget()), BootTargetIdentifier> &&
          std::is_same_v<decltype(std::declval<const T&>().CommittedBootTarget()), BootTargetIdentifier> &&
          std::is_same_v<decltype(std::declval<const T&>().NextBootTarget()), BootTargetIdentifier> &&
          std::is_same_v<decltype(std::declval<T&>().SelectNextBootTarget(BootTargetIdentifier{})), Result> &&
          noexcept(std::declval<const T&>().CurrentBootTarget()) &&
          noexcept(std::declval<const T&>().CommittedBootTarget()) &&
          noexcept(std::declval<const T&>().NextBootTarget()) &&
          noexcept(std::declval<T&>().SelectNextBootTarget(BootTargetIdentifier{}))> {};

template <typename T, typename = void>
struct IsTrialBootProvider : std::false_type {};

template <typename T>
struct IsTrialBootProvider<T, std::void_t<
    typename T::PlatformCapabilities,
    decltype(std::declval<const T&>().IsCurrentBootTrial()),
    decltype(std::declval<T&>().MarkCurrentBootValid()),
    decltype(std::declval<T&>().MarkCurrentBootInvalid())>>
    : std::bool_constant<
          IsProviderV<T> &&
          T::PlatformCapabilities::template Contains<Capability::TrialBoot> &&
          std::is_same_v<decltype(std::declval<const T&>().IsCurrentBootTrial()), bool> &&
          std::is_same_v<decltype(std::declval<T&>().MarkCurrentBootValid()), Result> &&
          std::is_same_v<decltype(std::declval<T&>().MarkCurrentBootInvalid()), Result> &&
          noexcept(std::declval<const T&>().IsCurrentBootTrial()) &&
          noexcept(std::declval<T&>().MarkCurrentBootValid()) &&
          noexcept(std::declval<T&>().MarkCurrentBootInvalid())> {};

template <typename T, typename = void>
struct HasTrialBootStateInspection : std::false_type {};

template <typename T>
struct HasTrialBootStateInspection<T, std::void_t<
    decltype(std::declval<const T&>().InspectBootTargetTrialState(BootTargetIdentifier{}))>>
    : std::bool_constant<
          IsProviderV<T> &&
          T::PlatformCapabilities::template Contains<Capability::TrialBoot> &&
          std::is_same_v<decltype(std::declval<const T&>().InspectBootTargetTrialState(BootTargetIdentifier{})), TrialBootStateResult> &&
          noexcept(std::declval<const T&>().InspectBootTargetTrialState(BootTargetIdentifier{}))> {};

template <typename T, typename = void>
struct IsSystemRestartProvider : std::false_type {};

template <typename T>
struct IsSystemRestartProvider<T, std::void_t<
    typename T::PlatformCapabilities,
    decltype(std::declval<T&>().Restart(RestartReason{}))>>
    : std::bool_constant<
          IsProviderV<T> &&
          T::PlatformCapabilities::template Contains<Capability::SystemRestart> &&
          std::is_same_v<decltype(std::declval<T&>().Restart(RestartReason{})), Result> &&
          noexcept(std::declval<T&>().Restart(RestartReason{}))> {};

template <typename T, typename = void>
struct IsStorageLayoutInspectionProvider : std::false_type {};

template <typename T>
struct IsStorageLayoutInspectionProvider<T, std::void_t<
    typename T::PlatformCapabilities,
    decltype(std::declval<const T&>().InspectStorageLayout(std::declval<StorageLayoutInfo&>()))>>
    : std::bool_constant<
          IsProviderV<T> &&
          T::PlatformCapabilities::template Contains<Capability::StorageLayoutInspection> &&
          std::is_same_v<decltype(std::declval<const T&>().InspectStorageLayout(std::declval<StorageLayoutInfo&>())), Result> &&
          noexcept(std::declval<const T&>().InspectStorageLayout(std::declval<StorageLayoutInfo&>()))> {};

template <typename T, typename = void>
struct IsStorageLayoutTransitionProvider : std::false_type {};

template <typename T>
struct IsStorageLayoutTransitionProvider<T, std::void_t<
    typename T::PlatformCapabilities,
    decltype(std::declval<T&>().PreflightStorageLayoutTransition(StorageLayoutIdentifier{})),
    decltype(std::declval<T&>().BeginStorageLayoutTransition(StorageLayoutIdentifier{})),
    decltype(std::declval<T&>().AdvanceStorageLayoutTransition()),
    decltype(std::declval<T&>().FinalizeStorageLayoutTransition()),
    decltype(std::declval<T&>().AbortStorageLayoutTransition())>>
    : std::bool_constant<
          IsProviderV<T> &&
          T::PlatformCapabilities::template Contains<Capability::StorageLayoutTransition> &&
          std::is_same_v<decltype(std::declval<T&>().PreflightStorageLayoutTransition(StorageLayoutIdentifier{})), Result> &&
          std::is_same_v<decltype(std::declval<T&>().BeginStorageLayoutTransition(StorageLayoutIdentifier{})), Result> &&
          std::is_same_v<decltype(std::declval<T&>().AdvanceStorageLayoutTransition()), Result> &&
          std::is_same_v<decltype(std::declval<T&>().FinalizeStorageLayoutTransition()), Result> &&
          std::is_same_v<decltype(std::declval<T&>().AbortStorageLayoutTransition()), Result> &&
          noexcept(std::declval<T&>().PreflightStorageLayoutTransition(StorageLayoutIdentifier{})) &&
          noexcept(std::declval<T&>().BeginStorageLayoutTransition(StorageLayoutIdentifier{})) &&
          noexcept(std::declval<T&>().AdvanceStorageLayoutTransition()) &&
          noexcept(std::declval<T&>().FinalizeStorageLayoutTransition()) &&
          noexcept(std::declval<T&>().AbortStorageLayoutTransition())> {};

} // namespace Detail

template <typename T> inline constexpr bool IsApplicationImageStagingProviderV = Detail::IsApplicationImageStagingProvider<T>::value;
template <typename T> inline constexpr bool IsFilesystemImageStagingProviderV = Detail::IsFilesystemImageStagingProvider<T>::value;
template <typename T> inline constexpr bool IsBootControlProviderV = Detail::IsBootControlProvider<T>::value;
template <typename T> inline constexpr bool IsTrialBootProviderV = Detail::IsTrialBootProvider<T>::value;
template <typename T> inline constexpr bool HasTrialBootStateInspectionV = Detail::HasTrialBootStateInspection<T>::value;
template <typename T> inline constexpr bool IsSystemRestartProviderV = Detail::IsSystemRestartProvider<T>::value;
template <typename T> inline constexpr bool IsStorageLayoutInspectionProviderV = Detail::IsStorageLayoutInspectionProvider<T>::value;
template <typename T> inline constexpr bool IsStorageLayoutTransitionProviderV = Detail::IsStorageLayoutTransitionProvider<T>::value;

} // namespace OTA
} // namespace ESPressio::Platform
