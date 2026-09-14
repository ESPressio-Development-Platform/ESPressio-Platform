#include <cstddef>
#include <cstdint>

#include "ESPressio_Platform.hpp"

using namespace ESPressio::Platform;

namespace {

struct TestBackend final : Backend {};

struct OTAProvider final : ProviderDeclaration<
    TestBackend,
    CapabilitySet<
        Capability::ApplicationImageStaging,
        Capability::FilesystemImageStaging,
        Capability::BootControl,
        Capability::TrialBoot,
        Capability::SystemRestart,
        Capability::StorageLayoutInspection,
        Capability::StorageLayoutTransition>> {

    OTA::BootTargetIdentifier current{1U};
    OTA::BootTargetIdentifier committed{1U};
    OTA::BootTargetIdentifier next{1U};
    OTA::BootTargetIdentifier candidate{2U};
    bool trial{false};

    OTA::PreflightResult PreflightApplicationImage(std::uint64_t bytes) noexcept {
        return {bytes <= 4096U ? OTA::Status::Success : OTA::Status::CapacityUnavailable,
                bytes, 4096U, 0};
    }
    OTA::Result BeginApplicationImage(std::uint64_t) noexcept { return {OTA::Status::Success, 0}; }
    OTA::WriteResult WriteApplicationImage(const std::uint8_t*, std::size_t bytes) noexcept {
        return {OTA::Status::Success, bytes, 0};
    }
    OTA::Result FinalizeApplicationImage() noexcept { return {OTA::Status::Success, 0}; }
    OTA::Result AbortApplicationImage() noexcept { return {OTA::Status::Success, 0}; }
    OTA::BootTargetIdentifier ApplicationImageTarget() const noexcept { return candidate; }

    OTA::PreflightResult PreflightFilesystemImage(std::uint64_t bytes) noexcept {
        return {bytes <= 2048U ? OTA::Status::Success : OTA::Status::CapacityUnavailable,
                bytes, 2048U, 0};
    }
    OTA::Result BeginFilesystemImage(std::uint64_t) noexcept { return {OTA::Status::Success, 0}; }
    OTA::WriteResult WriteFilesystemImage(const std::uint8_t*, std::size_t bytes) noexcept {
        return {OTA::Status::Success, bytes, 0};
    }
    OTA::Result FinalizeFilesystemImage() noexcept { return {OTA::Status::Success, 0}; }
    OTA::Result AbortFilesystemImage() noexcept { return {OTA::Status::Success, 0}; }

    OTA::BootTargetIdentifier CurrentBootTarget() const noexcept { return current; }
    OTA::BootTargetIdentifier CommittedBootTarget() const noexcept { return committed; }
    OTA::BootTargetIdentifier NextBootTarget() const noexcept { return next; }
    OTA::Result SelectNextBootTarget(OTA::BootTargetIdentifier target) noexcept {
        if (!target) return {OTA::Status::Invalid, 0};
        next = target;
        return {OTA::Status::Success, 0};
    }

    bool IsCurrentBootTrial() const noexcept { return trial; }
    OTA::Result MarkCurrentBootValid() noexcept {
        committed = current;
        trial = false;
        return {OTA::Status::Success, 0};
    }
    OTA::Result MarkCurrentBootInvalid() noexcept {
        trial = false;
        return {OTA::Status::Success, 0};
    }

    OTA::Result Restart(OTA::RestartReason) noexcept { return {OTA::Status::Success, 0}; }

    OTA::Result InspectStorageLayout(OTA::StorageLayoutInfo& info) const noexcept {
        info = {OTA::StorageLayoutIdentifier{7U}, 8192U, 4096U};
        return {OTA::Status::Success, 0};
    }

    OTA::Result PreflightStorageLayoutTransition(OTA::StorageLayoutIdentifier layout) noexcept {
        return {layout ? OTA::Status::Success : OTA::Status::Invalid, 0};
    }
    OTA::Result BeginStorageLayoutTransition(OTA::StorageLayoutIdentifier layout) noexcept {
        return {layout ? OTA::Status::Success : OTA::Status::Invalid, 0};
    }
    OTA::Result AdvanceStorageLayoutTransition() noexcept { return {OTA::Status::Success, 0}; }
    OTA::Result FinalizeStorageLayoutTransition() noexcept { return {OTA::Status::Success, 0}; }
    OTA::Result AbortStorageLayoutTransition() noexcept { return {OTA::Status::Success, 0}; }
};

static_assert(OTA::IsApplicationImageStagingProviderV<OTAProvider>);
static_assert(OTA::IsFilesystemImageStagingProviderV<OTAProvider>);
static_assert(OTA::IsBootControlProviderV<OTAProvider>);
static_assert(OTA::IsTrialBootProviderV<OTAProvider>);
static_assert(OTA::IsSystemRestartProviderV<OTAProvider>);
static_assert(OTA::IsStorageLayoutInspectionProviderV<OTAProvider>);
static_assert(OTA::IsStorageLayoutTransitionProviderV<OTAProvider>);

using TestPlatform = Composition<OTAProvider>;
static_assert(TestPlatform::IsValid);
static_assert(TestPlatform::Provides<Capability::ApplicationImageStaging>);
static_assert(TestPlatform::Provides<Capability::FilesystemImageStaging>);
static_assert(TestPlatform::Provides<Capability::BootControl>);
static_assert(TestPlatform::Provides<Capability::TrialBoot>);
static_assert(TestPlatform::Provides<Capability::SystemRestart>);
static_assert(TestPlatform::Provides<Capability::StorageLayoutInspection>);
static_assert(TestPlatform::Provides<Capability::StorageLayoutTransition>);

} // namespace

int main() {
    OTAProvider provider;
    if (!provider.PreflightApplicationImage(4096U)) return 1;
    if (provider.PreflightApplicationImage(4097U).Code != OTA::Status::CapacityUnavailable) return 2;
    if (!provider.SelectNextBootTarget(OTA::BootTargetIdentifier{2U})) return 3;
    if (provider.NextBootTarget() != OTA::BootTargetIdentifier{2U}) return 4;
    OTA::StorageLayoutInfo layout{};
    if (!provider.InspectStorageLayout(layout)) return 5;
    return layout.Layout == OTA::StorageLayoutIdentifier{7U} ? 0 : 6;
}
