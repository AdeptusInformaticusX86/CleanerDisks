#ifdef PLATFORM_WINDOWS

#include "core/disk_operations.hpp"
#include <windows.h>
#include <winioctl.h>
#include <vector>
#include <random>

namespace cleaner {
namespace disk_ops {
namespace impl {

std::vector<platform::DiskInfo> enumerate_disks() {
    std::vector<platform::DiskInfo> disks;

    // Enumerate logical drives
    DWORD drives = GetLogicalDrives();

    for (char drive = 'A'; drive <= 'Z'; ++drive) {
        if (drives & (1 << (drive - 'A'))) {
            std::string drive_path = std::string(1, drive) + ":\\";

            platform::DiskInfo info;
            info.device_path = drive_path;

            UINT type = GetDriveTypeA(drive_path.c_str());
            info.is_removable = (type == DRIVE_REMOVABLE || type == DRIVE_CDROM);

            ULARGE_INTEGER freeBytesAvailable, totalNumberOfBytes, totalNumberOfFreeBytes;
            if (GetDiskFreeSpaceExA(drive_path.c_str(), &freeBytesAvailable,
                                   &totalNumberOfBytes, &totalNumberOfFreeBytes)) {
                info.total_size = totalNumberOfBytes.QuadPart;
                info.free_size = totalNumberOfFreeBytes.QuadPart;
            }

            char volumeName[MAX_PATH];
            char fileSystemName[MAX_PATH];
            if (GetVolumeInformationA(drive_path.c_str(), volumeName, MAX_PATH,
                                     nullptr, nullptr, nullptr,
                                     fileSystemName, MAX_PATH)) {
                info.filesystem = fileSystemName;
            }

            info.mount_point = drive_path;

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

    UINT type = GetDriveTypeA(device_path.c_str());
    if (type == DRIVE_NO_ROOT_DIR || type == DRIVE_UNKNOWN) {
        return platform::ResultCode::ERROR_DEVICE_NOT_FOUND;
    }

    info.is_removable = (type == DRIVE_REMOVABLE || type == DRIVE_CDROM);

    ULARGE_INTEGER freeBytesAvailable, totalNumberOfBytes, totalNumberOfFreeBytes;
    if (GetDiskFreeSpaceExA(device_path.c_str(), &freeBytesAvailable,
                           &totalNumberOfBytes, &totalNumberOfFreeBytes)) {
        info.total_size = totalNumberOfBytes.QuadPart;
        info.free_size = totalNumberOfFreeBytes.QuadPart;
    }

    char volumeName[MAX_PATH];
    char fileSystemName[MAX_PATH];
    if (GetVolumeInformationA(device_path.c_str(), volumeName, MAX_PATH,
                             nullptr, nullptr, nullptr,
                             fileSystemName, MAX_PATH)) {
        info.filesystem = fileSystemName;
    }

    info.mount_point = device_path;

    return platform::ResultCode::SUCCESS;
}

platform::ResultCode unmount_disk_impl(const std::string& device_path) {
    // On Windows, we can't easily unmount a drive letter
    // This would require more complex operations
    return platform::ResultCode::ERROR_NOT_SUPPORTED;
}

platform::ResultCode mount_disk_impl(
    const std::string& device_path,
    const std::string& mount_point
) {
    return platform::ResultCode::ERROR_NOT_SUPPORTED;
}

platform::ResultCode unlock_disk_impl(const std::string& device_path) {
    std::string volume_path = "\\\\.\\" + device_path.substr(0, 2);

    HANDLE hVolume = CreateFileA(
        volume_path.c_str(),
        GENERIC_READ | GENERIC_WRITE,
        FILE_SHARE_READ | FILE_SHARE_WRITE,
        nullptr,
        OPEN_EXISTING,
        0,
        nullptr
    );

    if (hVolume == INVALID_HANDLE_VALUE) {
        return platform::ResultCode::ERROR_PERMISSION_DENIED;
    }

    DWORD bytesReturned;
    BOOL result = DeviceIoControl(
        hVolume,
        IOCTL_DISK_UPDATE_PROPERTIES,
        nullptr, 0,
        nullptr, 0,
        &bytesReturned,
        nullptr
    );

    CloseHandle(hVolume);

    return result ? platform::ResultCode::SUCCESS : platform::ResultCode::ERROR_IO_FAILURE;
}

platform::ResultCode format_disk_impl(
    const std::string& device_path,
    const FormatOptions& options,
    ProgressCallback progress
) {
    // Windows formatting requires calling format.com or using complex APIs
    // For now, return not supported
    // Full implementation would use SHFormatDrive or format command
    return platform::ResultCode::ERROR_NOT_SUPPORTED;
}

platform::ResultCode wipe_disk_impl(
    const std::string& device_path,
    uint32_t passes,
    ProgressCallback progress
) {
    std::string volume_path = "\\\\.\\" + device_path.substr(0, 2);

    HANDLE hVolume = CreateFileA(
        volume_path.c_str(),
        GENERIC_WRITE,
        FILE_SHARE_READ | FILE_SHARE_WRITE,
        nullptr,
        OPEN_EXISTING,
        0,
        nullptr
    );

    if (hVolume == INVALID_HANDLE_VALUE) {
        return platform::ResultCode::ERROR_PERMISSION_DENIED;
    }

    GET_LENGTH_INFORMATION lengthInfo;
    DWORD bytesReturned;

    if (!DeviceIoControl(hVolume, IOCTL_DISK_GET_LENGTH_INFO,
                        nullptr, 0, &lengthInfo, sizeof(lengthInfo),
                        &bytesReturned, nullptr)) {
        CloseHandle(hVolume);
        return platform::ResultCode::ERROR_IO_FAILURE;
    }

    uint64_t device_size = lengthInfo.Length.QuadPart;

    constexpr size_t buffer_size = platform::DEFAULT_BUFFER_SIZE;
    std::vector<uint8_t> buffer(buffer_size);

    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(0, 255);

    for (uint32_t pass = 0; pass < passes; ++pass) {
        SetFilePointer(hVolume, 0, nullptr, FILE_BEGIN);

        uint64_t bytes_written = 0;

        while (bytes_written < device_size) {
            DWORD to_write = static_cast<DWORD>(
                std::min(buffer_size, static_cast<size_t>(device_size - bytes_written))
            );

            // Fill buffer
            if (pass % 3 == 0) {
                memset(buffer.data(), 0x00, to_write);
            } else if (pass % 3 == 1) {
                memset(buffer.data(), 0xFF, to_write);
            } else {
                for (size_t i = 0; i < to_write; ++i) {
                    buffer[i] = static_cast<uint8_t>(dis(gen));
                }
            }

            DWORD written;
            if (!WriteFile(hVolume, buffer.data(), to_write, &written, nullptr)) {
                CloseHandle(hVolume);
                return platform::ResultCode::ERROR_IO_FAILURE;
            }

            bytes_written += written;

            if (progress) {
                uint64_t total_progress = (pass * device_size) + bytes_written;
                uint64_t total_size = passes * device_size;
                progress(total_progress, total_size);
            }
        }

        FlushFileBuffers(hVolume);
    }

    CloseHandle(hVolume);
    return platform::ResultCode::SUCCESS;
}

} // namespace impl
} // namespace disk_ops
} // namespace cleaner

#endif // PLATFORM_WINDOWS
