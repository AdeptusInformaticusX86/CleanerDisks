#ifdef PLATFORM_MACOS

#include "core/disk_operations.hpp"
#include <sys/param.h>
#include <sys/mount.h>
#include <sys/disk.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <unistd.h>
#include <vector>
#include <random>
#include <cstring>

// macOS-specific includes
#include <CoreFoundation/CoreFoundation.h>
#include <DiskArbitration/DiskArbitration.h>
#include <IOKit/IOKitLib.h>
#include <IOKit/storage/IOMedia.h>

namespace cleaner {
namespace disk_ops {
namespace impl {

std::vector<platform::DiskInfo> enumerate_disks() {
    std::vector<platform::DiskInfo> disks;

    // Use DiskArbitration framework to enumerate disks
    // This is a simplified version - full implementation would use DiskArbitration API

    struct statfs* mounts;
    int count = getmntinfo(&mounts, MNT_NOWAIT);

    for (int i = 0; i < count; ++i) {
        platform::DiskInfo info;
        info.device_path = mounts[i].f_mntfromname;
        info.mount_point = mounts[i].f_mntonname;
        info.filesystem = mounts[i].f_fstypename;
        info.total_size = mounts[i].f_blocks * mounts[i].f_bsize;
        info.free_size = mounts[i].f_bavail * mounts[i].f_bsize;

        // Check if removable
        info.is_removable = (mounts[i].f_flags & MNT_REMOVABLE) != 0;
        info.is_read_only = (mounts[i].f_flags & MNT_RDONLY) != 0;

        disks.push_back(info);
    }

    return disks;
}

platform::ResultCode get_disk_info_impl(
    const std::string& device_path,
    platform::DiskInfo& info
) {
    info.device_path = device_path;

    // Try to get mount info
    struct statfs* mounts;
    int count = getmntinfo(&mounts, MNT_NOWAIT);

    for (int i = 0; i < count; ++i) {
        if (device_path == mounts[i].f_mntfromname) {
            info.mount_point = mounts[i].f_mntonname;
            info.filesystem = mounts[i].f_fstypename;
            info.total_size = mounts[i].f_blocks * mounts[i].f_bsize;
            info.free_size = mounts[i].f_bavail * mounts[i].f_bsize;
            info.is_removable = (mounts[i].f_flags & MNT_REMOVABLE) != 0;
            info.is_read_only = (mounts[i].f_flags & MNT_RDONLY) != 0;

            return platform::ResultCode::SUCCESS;
        }
    }

    // Try to open device to get size
    int fd = open(device_path.c_str(), O_RDONLY);
    if (fd >= 0) {
        uint64_t block_count;
        uint32_t block_size;

        if (ioctl(fd, DKIOCGETBLOCKCOUNT, &block_count) == 0 &&
            ioctl(fd, DKIOCGETBLOCKSIZE, &block_size) == 0) {
            info.total_size = block_count * block_size;
        }

        close(fd);
        return platform::ResultCode::SUCCESS;
    }

    return platform::ResultCode::ERROR_DEVICE_NOT_FOUND;
}

platform::ResultCode unmount_disk_impl(const std::string& device_path) {
    platform::DiskInfo info;
    auto result = get_disk_info_impl(device_path, info);
    if (result != platform::ResultCode::SUCCESS) {
        return result;
    }

    if (info.mount_point.empty()) {
        return platform::ResultCode::SUCCESS;
    }

    if (unmount(info.mount_point.c_str(), 0) != 0) {
        if (errno == EACCES || errno == EPERM) {
            return platform::ResultCode::ERROR_PERMISSION_DENIED;
        } else if (errno == EBUSY) {
            // Try force unmount
            if (unmount(info.mount_point.c_str(), MNT_FORCE) != 0) {
                return platform::ResultCode::ERROR_DEVICE_BUSY;
            }
        } else {
            return platform::ResultCode::ERROR_IO_FAILURE;
        }
    }

    return platform::ResultCode::SUCCESS;
}

platform::ResultCode mount_disk_impl(
    const std::string& device_path,
    const std::string& mount_point
) {
    // macOS uses diskutil or DiskArbitration for mounting
    // Direct mount() call is more complex on macOS
    return platform::ResultCode::ERROR_NOT_SUPPORTED;
}

platform::ResultCode unlock_disk_impl(const std::string& device_path) {
    // On macOS, unlocking might involve using diskutil
    // or IOKit to modify device properties
    return platform::ResultCode::ERROR_NOT_SUPPORTED;
}

platform::ResultCode format_disk_impl(
    const std::string& device_path,
    const FormatOptions& options,
    ProgressCallback progress
) {
    // macOS formatting should use diskutil command
    std::string command = "diskutil eraseDisk ";

    switch (options.filesystem) {
        case FilesystemType::APFS:
            command += "APFS ";
            break;
        case FilesystemType::HFS_PLUS:
            command += "JHFS+ ";
            break;
        case FilesystemType::FAT32:
            command += "FAT32 ";
            break;
        case FilesystemType::EXFAT:
            command += "ExFAT ";
            break;
        default:
            command += "APFS ";
            break;
    }

    command += (options.label.empty() ? "Untitled" : options.label) + " ";
    command += device_path;

    int ret = system(command.c_str());
    if (ret != 0) {
        return platform::ResultCode::ERROR_IO_FAILURE;
    }

    if (progress) {
        progress(100, 100);
    }

    return platform::ResultCode::SUCCESS;
}

platform::ResultCode wipe_disk_impl(
    const std::string& device_path,
    uint32_t passes,
    ProgressCallback progress
) {
    int fd = open(device_path.c_str(), O_WRONLY);
    if (fd < 0) {
        if (errno == EACCES || errno == EPERM) {
            return platform::ResultCode::ERROR_PERMISSION_DENIED;
        }
        return platform::ResultCode::ERROR_DEVICE_NOT_FOUND;
    }

    uint64_t block_count;
    uint32_t block_size;

    if (ioctl(fd, DKIOCGETBLOCKCOUNT, &block_count) != 0 ||
        ioctl(fd, DKIOCGETBLOCKSIZE, &block_size) != 0) {
        close(fd);
        return platform::ResultCode::ERROR_IO_FAILURE;
    }

    uint64_t device_size = block_count * block_size;

    constexpr size_t buffer_size = platform::DEFAULT_BUFFER_SIZE;
    std::vector<uint8_t> buffer(buffer_size);

    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(0, 255);

    for (uint32_t pass = 0; pass < passes; ++pass) {
        lseek(fd, 0, SEEK_SET);

        uint64_t bytes_written = 0;

        while (bytes_written < device_size) {
            size_t to_write = std::min(buffer_size,
                                      static_cast<size_t>(device_size - bytes_written));

            if (pass % 3 == 0) {
                std::memset(buffer.data(), 0x00, to_write);
            } else if (pass % 3 == 1) {
                std::memset(buffer.data(), 0xFF, to_write);
            } else {
                for (size_t i = 0; i < to_write; ++i) {
                    buffer[i] = static_cast<uint8_t>(dis(gen));
                }
            }

            ssize_t written = write(fd, buffer.data(), to_write);
            if (written < 0) {
                close(fd);
                return platform::ResultCode::ERROR_IO_FAILURE;
            }

            bytes_written += written;

            if (progress) {
                uint64_t total_progress = (pass * device_size) + bytes_written;
                uint64_t total_size = passes * device_size;
                progress(total_progress, total_size);
            }
        }

        fcntl(fd, F_FULLFSYNC);
    }

    close(fd);
    return platform::ResultCode::SUCCESS;
}

} // namespace impl
} // namespace disk_ops
} // namespace cleaner

#endif // PLATFORM_MACOS
