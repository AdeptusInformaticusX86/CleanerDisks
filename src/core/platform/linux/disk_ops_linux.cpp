#include "core/disk_operations.hpp"
#include <fstream>
#include <sstream>
#include <random>
#include <cstring>
#include <vector>

// Linux-specific includes
#include <sys/ioctl.h>
#include <sys/mount.h>
#include <sys/stat.h>
#include <sys/statvfs.h>
#include <linux/fs.h>
#include <fcntl.h>
#include <unistd.h>
#include <mntent.h>
#include <blkid/blkid.h>

namespace cleaner {
namespace disk_ops {
namespace impl {

std::vector<platform::DiskInfo> enumerate_disks() {
    std::vector<platform::DiskInfo> disks;

    // Read /proc/partitions
    std::ifstream partitions("/proc/partitions");
    if (!partitions.is_open()) {
        return disks;
    }

    std::string line;
    std::getline(partitions, line); // Skip header
    std::getline(partitions, line); // Skip header

    while (std::getline(partitions, line)) {
        std::istringstream iss(line);
        int major, minor;
        uint64_t blocks;
        std::string name;

        if (iss >> major >> minor >> blocks >> name) {
            // Skip ram, loop, and dm devices
            if (name.find("ram") == 0 || name.find("loop") == 0 || name.find("dm-") == 0) {
                continue;
            }

            platform::DiskInfo info;
            info.device_path = "/dev/" + name;
            info.total_size = blocks * 1024; // blocks are in 1K units

            // Try to get more info
            get_disk_info_impl(info.device_path, info);

            disks.push_back(info);
        }
    }

    return disks;
}

platform::ResultCode get_disk_info_impl(
    const std::string& device_path,
    platform::DiskInfo& info
) {
    info.device_path = device_path;

    // Check if device exists
    struct stat st;
    if (stat(device_path.c_str(), &st) != 0) {
        return platform::ResultCode::ERROR_DEVICE_NOT_FOUND;
    }

    // Check if it's a block device
    if (!S_ISBLK(st.st_mode)) {
        return platform::ResultCode::ERROR_DEVICE_NOT_FOUND;
    }

    // Get size
    int fd = open(device_path.c_str(), O_RDONLY);
    if (fd >= 0) {
        uint64_t size;
        if (ioctl(fd, BLKGETSIZE64, &size) == 0) {
            info.total_size = size;
        }

        // Check if read-only
        int ro;
        if (ioctl(fd, BLKROGET, &ro) == 0) {
            info.is_read_only = (ro != 0);
        }

        close(fd);
    }

    // Check if removable
    std::string dev_name = device_path.substr(device_path.find_last_of('/') + 1);
    // Remove partition number if present
    while (!dev_name.empty() && std::isdigit(dev_name.back())) {
        dev_name.pop_back();
    }

    std::string removable_path = "/sys/block/" + dev_name + "/removable";
    std::ifstream removable_file(removable_path);
    if (removable_file.is_open()) {
        int removable;
        removable_file >> removable;
        info.is_removable = (removable == 1);
    }

    // Get filesystem type using blkid
    blkid_cache cache;
    if (blkid_get_cache(&cache, nullptr) == 0) {
        const char* type = blkid_get_tag_value(cache, "TYPE", device_path.c_str());
        if (type) {
            info.filesystem = type;
        }
        blkid_put_cache(cache);
    }

    // Check if mounted
    FILE* mtab = setmntent("/etc/mtab", "r");
    if (mtab) {
        struct mntent* entry;
        while ((entry = getmntent(mtab)) != nullptr) {
            if (device_path == entry->mnt_fsname) {
                info.mount_point = entry->mnt_dir;
                break;
            }
        }
        endmntent(mtab);
    }

    // Get free space if mounted
    if (!info.mount_point.empty()) {
        struct statvfs vfs;
        if (statvfs(info.mount_point.c_str(), &vfs) == 0) {
            info.free_size = vfs.f_bsize * vfs.f_bavail;
        }
    }

    return platform::ResultCode::SUCCESS;
}

platform::ResultCode unmount_disk_impl(const std::string& device_path) {
    platform::DiskInfo info;
    auto result = get_disk_info_impl(device_path, info);
    if (result != platform::ResultCode::SUCCESS) {
        return result;
    }

    if (info.mount_point.empty()) {
        return platform::ResultCode::SUCCESS; // Already unmounted
    }

    if (umount(info.mount_point.c_str()) != 0) {
        if (errno == EACCES || errno == EPERM) {
            return platform::ResultCode::ERROR_PERMISSION_DENIED;
        } else if (errno == EBUSY) {
            // Try lazy unmount
            if (umount2(info.mount_point.c_str(), MNT_DETACH) != 0) {
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
    // Get filesystem type
    platform::DiskInfo info;
    auto result = get_disk_info_impl(device_path, info);
    if (result != platform::ResultCode::SUCCESS) {
        return result;
    }

    if (info.filesystem.empty()) {
        return platform::ResultCode::ERROR_INVALID_FILESYSTEM;
    }

    if (mount(device_path.c_str(), mount_point.c_str(),
              info.filesystem.c_str(), 0, nullptr) != 0) {
        if (errno == EACCES || errno == EPERM) {
            return platform::ResultCode::ERROR_PERMISSION_DENIED;
        }
        return platform::ResultCode::ERROR_IO_FAILURE;
    }

    return platform::ResultCode::SUCCESS;
}

platform::ResultCode unlock_disk_impl(const std::string& device_path) {
    // Get device base name (remove partition number)
    std::string dev_name = device_path.substr(device_path.find_last_of('/') + 1);
    std::string dev_base = dev_name;
    while (!dev_base.empty() && std::isdigit(dev_base.back())) {
        dev_base.pop_back();
    }

    // Check current read-only status
    int fd = open(device_path.c_str(), O_RDONLY);
    if (fd < 0) {
        if (errno == EACCES || errno == EPERM) {
            return platform::ResultCode::ERROR_PERMISSION_DENIED;
        }
        return platform::ResultCode::ERROR_DEVICE_NOT_FOUND;
    }

    int ro_current;
    if (ioctl(fd, BLKROGET, &ro_current) == 0) {
        if (ro_current == 0) {
            close(fd);
            return platform::ResultCode::SUCCESS; // Already unlocked
        }
    }
    close(fd);

    // Check for hardware write-protect switch
    std::string wp_path = "/sys/block/" + dev_base + "/device/wp";
    std::ifstream wp_file(wp_path);
    if (wp_file.is_open()) {
        int wp;
        wp_file >> wp;
        if (wp == 1) {
            // Hardware write-protect is enabled
            return platform::ResultCode::ERROR_NOT_SUPPORTED;
        }
    }

    // Method 1: Try BLKROSET
    fd = open(device_path.c_str(), O_RDWR);
    if (fd >= 0) {
        int ro = 0;
        if (ioctl(fd, BLKROSET, &ro) == 0) {
            // Verify it worked
            int ro_verify;
            if (ioctl(fd, BLKROGET, &ro_verify) == 0 && ro_verify == 0) {
                close(fd);
                return platform::ResultCode::SUCCESS;
            }
        }
        close(fd);
    }

    // Method 2: Try using hdparm command
    std::string cmd = "hdparm -r0 " + device_path + " >/dev/null 2>&1";
    if (system(cmd.c_str()) == 0) {
        // Verify it worked
        fd = open(device_path.c_str(), O_RDONLY);
        if (fd >= 0) {
            int ro_verify;
            if (ioctl(fd, BLKROGET, &ro_verify) == 0 && ro_verify == 0) {
                close(fd);
                return platform::ResultCode::SUCCESS;
            }
            close(fd);
        }
    }

    // Method 3: Try blockdev --setrw
    cmd = "blockdev --setrw " + device_path + " >/dev/null 2>&1";
    if (system(cmd.c_str()) == 0) {
        // Verify it worked
        fd = open(device_path.c_str(), O_RDONLY);
        if (fd >= 0) {
            int ro_verify;
            if (ioctl(fd, BLKROGET, &ro_verify) == 0 && ro_verify == 0) {
                close(fd);
                return platform::ResultCode::SUCCESS;
            }
            close(fd);
        }
    }

    // Method 4: Try USB reset
    std::string usb_reset_cmd = "udevadm info --query=path --name=" + device_path +
                                " | sed 's|/block/.*||' | while read path; do " +
                                "if [ -f \"/sys$path/authorized\" ]; then " +
                                "echo 0 > /sys$path/authorized; sleep 1; " +
                                "echo 1 > /sys$path/authorized; sleep 1; " +
                                "fi; done 2>/dev/null";
    system(usb_reset_cmd.c_str());

    // Final verification
    fd = open(device_path.c_str(), O_RDONLY);
    if (fd >= 0) {
        int ro_final;
        if (ioctl(fd, BLKROGET, &ro_final) == 0) {
            close(fd);
            if (ro_final == 0) {
                return platform::ResultCode::SUCCESS;
            }
        } else {
            close(fd);
        }
    }

    // All methods failed
    return platform::ResultCode::ERROR_IO_FAILURE;
}

platform::ResultCode format_disk_impl(
    const std::string& device_path,
    const FormatOptions& options,
    ProgressCallback progress
) {
    // Determine filesystem command
    std::string command;
    std::string fs_type;

    switch (options.filesystem) {
        case FilesystemType::EXT4:
            command = "mkfs.ext4 -F ";
            fs_type = "ext4";
            break;
        case FilesystemType::NTFS:
            command = "mkfs.ntfs -f ";
            fs_type = "ntfs";
            break;
        case FilesystemType::FAT32:
            command = "mkfs.vfat -F 32 ";
            fs_type = "fat32";
            break;
        case FilesystemType::EXFAT:
            command = "mkfs.exfat ";
            fs_type = "exfat";
            break;
        default:
            command = "mkfs.ext4 -F ";
            fs_type = "ext4";
            break;
    }

    if (!options.label.empty()) {
        command += "-L \"" + options.label + "\" ";
    }

    command += device_path;

    // Execute format command
    int ret = system(command.c_str());
    if (ret != 0) {
        if (WEXITSTATUS(ret) == 127) {
            return platform::ResultCode::ERROR_NOT_SUPPORTED;
        }
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
    // Open device
    int fd = open(device_path.c_str(), O_WRONLY);
    if (fd < 0) {
        if (errno == EACCES || errno == EPERM) {
            return platform::ResultCode::ERROR_PERMISSION_DENIED;
        }
        return platform::ResultCode::ERROR_DEVICE_NOT_FOUND;
    }

    // Get device size
    uint64_t device_size;
    if (ioctl(fd, BLKGETSIZE64, &device_size) != 0) {
        close(fd);
        return platform::ResultCode::ERROR_IO_FAILURE;
    }

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

            // Fill buffer based on pass
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

        fsync(fd);
    }

    close(fd);
    return platform::ResultCode::SUCCESS;
}

} // namespace impl
} // namespace disk_ops
} // namespace cleaner
