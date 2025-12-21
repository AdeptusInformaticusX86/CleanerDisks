#ifdef PLATFORM_WINDOWS

#include "core/file_operations.hpp"
#include <windows.h>
#include <random>
#include <vector>

namespace cleaner {
namespace file_ops {
namespace impl {

platform::ResultCode overwrite_file(
    const std::string& filepath,
    uint8_t pattern,
    ProgressCallback progress
) {
    // Open file with write access
    HANDLE hFile = CreateFileA(
        filepath.c_str(),
        GENERIC_WRITE | GENERIC_READ,
        0,
        nullptr,
        OPEN_EXISTING,
        FILE_ATTRIBUTE_NORMAL,
        nullptr
    );

    if (hFile == INVALID_HANDLE_VALUE) {
        DWORD error = GetLastError();
        if (error == ERROR_ACCESS_DENIED) {
            return platform::ResultCode::ERROR_PERMISSION_DENIED;
        }
        return platform::ResultCode::ERROR_DEVICE_NOT_FOUND;
    }

    // Get file size
    LARGE_INTEGER fileSize;
    if (!GetFileSizeEx(hFile, &fileSize)) {
        CloseHandle(hFile);
        return platform::ResultCode::ERROR_IO_FAILURE;
    }

    uint64_t file_size = fileSize.QuadPart;

    // Create buffer
    constexpr size_t buffer_size = platform::DEFAULT_BUFFER_SIZE;
    std::vector<uint8_t> buffer(buffer_size, pattern);

    uint64_t bytes_written = 0;
    SetFilePointer(hFile, 0, nullptr, FILE_BEGIN);

    while (bytes_written < file_size) {
        DWORD to_write = static_cast<DWORD>(
            std::min(buffer_size, static_cast<size_t>(file_size - bytes_written))
        );

        DWORD written;
        if (!WriteFile(hFile, buffer.data(), to_write, &written, nullptr)) {
            CloseHandle(hFile);
            return platform::ResultCode::ERROR_IO_FAILURE;
        }

        bytes_written += written;

        if (progress) {
            progress(bytes_written, file_size);
        }
    }

    FlushFileBuffers(hFile);
    CloseHandle(hFile);

    return platform::ResultCode::SUCCESS;
}

platform::ResultCode overwrite_file_random(
    const std::string& filepath,
    ProgressCallback progress
) {
    HANDLE hFile = CreateFileA(
        filepath.c_str(),
        GENERIC_WRITE | GENERIC_READ,
        0,
        nullptr,
        OPEN_EXISTING,
        FILE_ATTRIBUTE_NORMAL,
        nullptr
    );

    if (hFile == INVALID_HANDLE_VALUE) {
        DWORD error = GetLastError();
        if (error == ERROR_ACCESS_DENIED) {
            return platform::ResultCode::ERROR_PERMISSION_DENIED;
        }
        return platform::ResultCode::ERROR_DEVICE_NOT_FOUND;
    }

    LARGE_INTEGER fileSize;
    if (!GetFileSizeEx(hFile, &fileSize)) {
        CloseHandle(hFile);
        return platform::ResultCode::ERROR_IO_FAILURE;
    }

    uint64_t file_size = fileSize.QuadPart;

    constexpr size_t buffer_size = platform::DEFAULT_BUFFER_SIZE;
    std::vector<uint8_t> buffer(buffer_size);

    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(0, 255);

    uint64_t bytes_written = 0;
    SetFilePointer(hFile, 0, nullptr, FILE_BEGIN);

    while (bytes_written < file_size) {
        DWORD to_write = static_cast<DWORD>(
            std::min(buffer_size, static_cast<size_t>(file_size - bytes_written))
        );

        // Fill buffer with random data
        for (size_t i = 0; i < to_write; ++i) {
            buffer[i] = static_cast<uint8_t>(dis(gen));
        }

        DWORD written;
        if (!WriteFile(hFile, buffer.data(), to_write, &written, nullptr)) {
            CloseHandle(hFile);
            return platform::ResultCode::ERROR_IO_FAILURE;
        }

        bytes_written += written;

        if (progress) {
            progress(bytes_written, file_size);
        }
    }

    FlushFileBuffers(hFile);
    CloseHandle(hFile);

    return platform::ResultCode::SUCCESS;
}

platform::ResultCode delete_file(const std::string& filepath) {
    if (DeleteFileA(filepath.c_str())) {
        return platform::ResultCode::SUCCESS;
    }

    DWORD error = GetLastError();
    if (error == ERROR_ACCESS_DENIED) {
        return platform::ResultCode::ERROR_PERMISSION_DENIED;
    } else if (error == ERROR_FILE_NOT_FOUND) {
        return platform::ResultCode::ERROR_DEVICE_NOT_FOUND;
    }

    return platform::ResultCode::ERROR_IO_FAILURE;
}

platform::ResultCode delete_directory(const std::string& dirpath) {
    if (RemoveDirectoryA(dirpath.c_str())) {
        return platform::ResultCode::SUCCESS;
    }

    DWORD error = GetLastError();
    if (error == ERROR_ACCESS_DENIED) {
        return platform::ResultCode::ERROR_PERMISSION_DENIED;
    } else if (error == ERROR_FILE_NOT_FOUND || error == ERROR_PATH_NOT_FOUND) {
        return platform::ResultCode::ERROR_DEVICE_NOT_FOUND;
    }

    return platform::ResultCode::ERROR_IO_FAILURE;
}

} // namespace impl
} // namespace file_ops
} // namespace cleaner

#endif // PLATFORM_WINDOWS
