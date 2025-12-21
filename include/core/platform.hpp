#pragma once

#include <string>
#include <cstdint>

namespace cleaner {
namespace platform {

// Platform detection
#if defined(_WIN32) || defined(_WIN64)
    #define PLATFORM_WINDOWS
    #define PATH_SEPARATOR "\\"
#elif defined(__APPLE__) && defined(__MACH__)
    #define PLATFORM_MACOS
    #define PATH_SEPARATOR "/"
#elif defined(__linux__)
    #define PLATFORM_LINUX
    #define PATH_SEPARATOR "/"
#else
    #error "Unsupported platform"
#endif

// Platform-specific types
#ifdef PLATFORM_WINDOWS
    using DiskHandle = void*; // HANDLE
#else
    using DiskHandle = int; // file descriptor
#endif

// Constants
constexpr size_t DEFAULT_BUFFER_SIZE = 4096 * 1024; // 4MB
constexpr uint8_t ZERO_PATTERN = 0x00;
constexpr uint8_t ONE_PATTERN = 0xFF;
constexpr uint8_t RANDOM_PATTERN = 0xAA;

// Disk information structure
struct DiskInfo {
    std::string device_path;
    std::string mount_point;
    std::string filesystem;
    uint64_t total_size;
    uint64_t free_size;
    bool is_removable;
    bool is_read_only;
};

// Result codes
enum class ResultCode {
    SUCCESS = 0,
    ERROR_PERMISSION_DENIED,
    ERROR_DEVICE_NOT_FOUND,
    ERROR_DEVICE_BUSY,
    ERROR_DEVICE_READ_ONLY,
    ERROR_INVALID_FILESYSTEM,
    ERROR_IO_FAILURE,
    ERROR_NOT_SUPPORTED,
    ERROR_UNKNOWN
};

// Convert result code to string
inline const char* result_code_to_string(ResultCode code) {
    switch(code) {
        case ResultCode::SUCCESS: return "Success";
        case ResultCode::ERROR_PERMISSION_DENIED: return "Permission denied";
        case ResultCode::ERROR_DEVICE_NOT_FOUND: return "Device not found";
        case ResultCode::ERROR_DEVICE_BUSY: return "Device is busy";
        case ResultCode::ERROR_DEVICE_READ_ONLY: return "Device is read-only";
        case ResultCode::ERROR_INVALID_FILESYSTEM: return "Invalid filesystem";
        case ResultCode::ERROR_IO_FAILURE: return "I/O failure";
        case ResultCode::ERROR_NOT_SUPPORTED: return "Operation not supported";
        default: return "Unknown error";
    }
}

} // namespace platform
} // namespace cleaner
