#ifdef PLATFORM_MACOS

#include "core/file_operations.hpp"
#include <fstream>
#include <random>
#include <unistd.h>
#include <sys/stat.h>
#include <filesystem>

namespace fs = std::filesystem;

namespace cleaner {
namespace file_ops {
namespace impl {

// macOS implementation is similar to Linux for file operations

platform::ResultCode overwrite_file(
    const std::string& filepath,
    uint8_t pattern,
    ProgressCallback progress
) {
    std::fstream file(filepath, std::ios::in | std::ios::out | std::ios::binary);
    if (!file.is_open()) {
        return platform::ResultCode::ERROR_PERMISSION_DENIED;
    }

    file.seekg(0, std::ios::end);
    uint64_t file_size = file.tellg();
    file.seekg(0, std::ios::beg);

    constexpr size_t buffer_size = platform::DEFAULT_BUFFER_SIZE;
    std::vector<uint8_t> buffer(buffer_size, pattern);

    uint64_t bytes_written = 0;

    while (bytes_written < file_size) {
        size_t to_write = std::min(buffer_size, static_cast<size_t>(file_size - bytes_written));

        file.write(reinterpret_cast<char*>(buffer.data()), to_write);
        if (!file.good()) {
            return platform::ResultCode::ERROR_IO_FAILURE;
        }

        bytes_written += to_write;

        if (progress) {
            progress(bytes_written, file_size);
        }
    }

    file.flush();
    fcntl(fileno(fopen(filepath.c_str(), "r")), F_FULLFSYNC);

    return platform::ResultCode::SUCCESS;
}

platform::ResultCode overwrite_file_random(
    const std::string& filepath,
    ProgressCallback progress
) {
    std::fstream file(filepath, std::ios::in | std::ios::out | std::ios::binary);
    if (!file.is_open()) {
        return platform::ResultCode::ERROR_PERMISSION_DENIED;
    }

    file.seekg(0, std::ios::end);
    uint64_t file_size = file.tellg();
    file.seekg(0, std::ios::beg);

    constexpr size_t buffer_size = platform::DEFAULT_BUFFER_SIZE;
    std::vector<uint8_t> buffer(buffer_size);

    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(0, 255);

    uint64_t bytes_written = 0;

    while (bytes_written < file_size) {
        size_t to_write = std::min(buffer_size, static_cast<size_t>(file_size - bytes_written));

        for (size_t i = 0; i < to_write; ++i) {
            buffer[i] = static_cast<uint8_t>(dis(gen));
        }

        file.write(reinterpret_cast<char*>(buffer.data()), to_write);
        if (!file.good()) {
            return platform::ResultCode::ERROR_IO_FAILURE;
        }

        bytes_written += to_write;

        if (progress) {
            progress(bytes_written, file_size);
        }
    }

    file.flush();
    fcntl(fileno(fopen(filepath.c_str(), "r")), F_FULLFSYNC);

    return platform::ResultCode::SUCCESS;
}

platform::ResultCode delete_file(const std::string& filepath) {
    try {
        if (fs::remove(filepath)) {
            return platform::ResultCode::SUCCESS;
        } else {
            return platform::ResultCode::ERROR_DEVICE_NOT_FOUND;
        }
    } catch (const fs::filesystem_error& e) {
        if (e.code() == std::errc::permission_denied) {
            return platform::ResultCode::ERROR_PERMISSION_DENIED;
        }
        return platform::ResultCode::ERROR_IO_FAILURE;
    }
}

platform::ResultCode delete_directory(const std::string& dirpath) {
    try {
        if (fs::remove_all(dirpath) > 0) {
            return platform::ResultCode::SUCCESS;
        } else {
            return platform::ResultCode::ERROR_DEVICE_NOT_FOUND;
        }
    } catch (const fs::filesystem_error& e) {
        if (e.code() == std::errc::permission_denied) {
            return platform::ResultCode::ERROR_PERMISSION_DENIED;
        }
        return platform::ResultCode::ERROR_IO_FAILURE;
    }
}

} // namespace impl
} // namespace file_ops
} // namespace cleaner

#endif // PLATFORM_MACOS
