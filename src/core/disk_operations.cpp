#include "core/disk_operations.hpp"

namespace cleaner {
namespace disk_ops {

std::vector<platform::DiskInfo> list_disks() {
    return impl::enumerate_disks();
}

platform::ResultCode get_disk_info(
    const std::string& device_path,
    platform::DiskInfo& info
) {
    return impl::get_disk_info_impl(device_path, info);
}

bool is_disk_mounted(const std::string& device_path) {
    platform::DiskInfo info;
    auto result = get_disk_info(device_path, info);
    if (result != platform::ResultCode::SUCCESS) {
        return false;
    }
    return !info.mount_point.empty();
}

platform::ResultCode unmount_disk(const std::string& device_path) {
    return impl::unmount_disk_impl(device_path);
}

platform::ResultCode mount_disk(
    const std::string& device_path,
    const std::string& mount_point
) {
    return impl::mount_disk_impl(device_path, mount_point);
}

platform::ResultCode unlock_disk(const std::string& device_path) {
    return impl::unlock_disk_impl(device_path);
}

platform::ResultCode format_disk(
    const std::string& device_path,
    const FormatOptions& options,
    ProgressCallback progress
) {
    // Check if disk is mounted
    if (is_disk_mounted(device_path) && !options.force) {
        return platform::ResultCode::ERROR_DEVICE_BUSY;
    }

    // Unmount if necessary
    if (is_disk_mounted(device_path)) {
        auto result = unmount_disk(device_path);
        if (result != platform::ResultCode::SUCCESS) {
            return result;
        }
    }

    return impl::format_disk_impl(device_path, options, progress);
}

platform::ResultCode secure_wipe_disk(
    const std::string& device_path,
    uint32_t passes,
    ProgressCallback progress
) {
    // Check if disk is mounted
    if (is_disk_mounted(device_path)) {
        auto result = unmount_disk(device_path);
        if (result != platform::ResultCode::SUCCESS) {
            return result;
        }
    }

    return impl::wipe_disk_impl(device_path, passes, progress);
}

} // namespace disk_ops
} // namespace cleaner
