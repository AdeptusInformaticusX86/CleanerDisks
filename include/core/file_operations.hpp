#pragma once

#include "platform.hpp"
#include <string>
#include <vector>
#include <functional>

namespace cleaner {
namespace file_ops {

// Callback for progress reporting
// Parameters: current_bytes, total_bytes
using ProgressCallback = std::function<void(uint64_t, uint64_t)>;

// Secure deletion methods
enum class DeletionMethod {
    SIMPLE,           // Simple deletion (unlink)
    DOD_3_PASS,       // DoD 5220.22-M (3 passes)
    DOD_7_PASS,       // DoD 5220.22-M (7 passes)
    GUTMANN_35_PASS,  // Gutmann method (35 passes)
    RANDOM_PASS,      // Single pass with random data
    ZERO_PASS         // Single pass with zeros
};

// Secure delete a single file
platform::ResultCode secure_delete_file(
    const std::string& filepath,
    DeletionMethod method = DeletionMethod::DOD_3_PASS,
    ProgressCallback progress = nullptr
);

// Secure delete a directory recursively
platform::ResultCode secure_delete_directory(
    const std::string& dirpath,
    DeletionMethod method = DeletionMethod::DOD_3_PASS,
    ProgressCallback progress = nullptr
);

// Get file size
uint64_t get_file_size(const std::string& filepath);

// Check if file exists
bool file_exists(const std::string& filepath);

// Check if directory exists
bool directory_exists(const std::string& dirpath);

// List all files in directory (recursive)
std::vector<std::string> list_files_recursive(const std::string& dirpath);

// Platform-specific implementations (to be implemented in platform-specific files)
namespace impl {
    platform::ResultCode overwrite_file(
        const std::string& filepath,
        uint8_t pattern,
        ProgressCallback progress
    );

    platform::ResultCode overwrite_file_random(
        const std::string& filepath,
        ProgressCallback progress
    );

    platform::ResultCode delete_file(const std::string& filepath);

    platform::ResultCode delete_directory(const std::string& dirpath);
}

} // namespace file_ops
} // namespace cleaner
