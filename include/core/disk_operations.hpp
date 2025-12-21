#pragma once

#include "platform.hpp"
#include <string>
#include <vector>
#include <functional>

namespace cleaner {
namespace disk_ops {

// Callback for progress reporting
using ProgressCallback = std::function<void(uint64_t, uint64_t)>;

// Filesystem types
enum class FilesystemType {
    AUTO,       // Auto-detect
    EXT4,       // Linux ext4
    NTFS,       // Windows NTFS
    FAT32,      // FAT32
    EXFAT,      // exFAT
    APFS,       // Apple APFS
    HFS_PLUS    // Apple HFS+
};

// Format options
struct FormatOptions {
    FilesystemType filesystem = FilesystemType::AUTO;
    std::string label;
    bool quick_format = false;
    bool force = false;
};

// List all available disks
std::vector<platform::DiskInfo> list_disks();

// Get disk information
platform::ResultCode get_disk_info(
    const std::string& device_path,
    platform::DiskInfo& info
);

// Check if disk is mounted
bool is_disk_mounted(const std::string& device_path);

// Unmount disk
platform::ResultCode unmount_disk(const std::string& device_path);

// Mount disk
platform::ResultCode mount_disk(
    const std::string& device_path,
    const std::string& mount_point
);

// Remove write protection (unlock disk)
platform::ResultCode unlock_disk(const std::string& device_path);

// Format disk with specified filesystem
platform::ResultCode format_disk(
    const std::string& device_path,
    const FormatOptions& options,
    ProgressCallback progress = nullptr
);

// Secure wipe disk (overwrite all data)
platform::ResultCode secure_wipe_disk(
    const std::string& device_path,
    uint32_t passes = 1,
    ProgressCallback progress = nullptr
);

// Platform-specific implementations
namespace impl {
    std::vector<platform::DiskInfo> enumerate_disks();

    platform::ResultCode get_disk_info_impl(
        const std::string& device_path,
        platform::DiskInfo& info
    );

    platform::ResultCode unmount_disk_impl(const std::string& device_path);

    platform::ResultCode mount_disk_impl(
        const std::string& device_path,
        const std::string& mount_point
    );

    platform::ResultCode unlock_disk_impl(const std::string& device_path);

    platform::ResultCode format_disk_impl(
        const std::string& device_path,
        const FormatOptions& options,
        ProgressCallback progress
    );

    platform::ResultCode wipe_disk_impl(
        const std::string& device_path,
        uint32_t passes,
        ProgressCallback progress
    );
}

} // namespace disk_ops
} // namespace cleaner
