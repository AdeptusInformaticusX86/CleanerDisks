#include "core/file_operations.hpp"
#include <random>
#include <fstream>
#include <filesystem>

namespace fs = std::filesystem;

namespace cleaner {
namespace file_ops {

uint64_t get_file_size(const std::string& filepath) {
    try {
        return fs::file_size(filepath);
    } catch(...) {
        return 0;
    }
}

bool file_exists(const std::string& filepath) {
    return fs::exists(filepath) && fs::is_regular_file(filepath);
}

bool directory_exists(const std::string& dirpath) {
    return fs::exists(dirpath) && fs::is_directory(dirpath);
}

std::vector<std::string> list_files_recursive(const std::string& dirpath) {
    std::vector<std::string> files;

    try {
        for (const auto& entry : fs::recursive_directory_iterator(dirpath)) {
            if (entry.is_regular_file()) {
                files.push_back(entry.path().string());
            }
        }
    } catch(...) {
        // Ignore errors
    }

    return files;
}

platform::ResultCode secure_delete_file(
    const std::string& filepath,
    DeletionMethod method,
    ProgressCallback progress
) {
    if (!file_exists(filepath)) {
        return platform::ResultCode::ERROR_DEVICE_NOT_FOUND;
    }

    uint64_t file_size = get_file_size(filepath);
    uint64_t current_progress = 0;
    uint32_t passes = 0;

    // Determine number of passes
    switch(method) {
        case DeletionMethod::SIMPLE:
            passes = 0;
            break;
        case DeletionMethod::ZERO_PASS:
        case DeletionMethod::RANDOM_PASS:
            passes = 1;
            break;
        case DeletionMethod::DOD_3_PASS:
            passes = 3;
            break;
        case DeletionMethod::DOD_7_PASS:
            passes = 7;
            break;
        case DeletionMethod::GUTMANN_35_PASS:
            passes = 35;
            break;
    }

    // Perform overwrites
    if (passes == 0) {
        // Simple deletion
        return impl::delete_file(filepath);
    }

    // DoD 3-pass method
    if (method == DeletionMethod::DOD_3_PASS) {
        // Pass 1: zeros
        auto result = impl::overwrite_file(filepath, platform::ZERO_PATTERN,
            [&](uint64_t curr, uint64_t total) {
                if (progress) progress(current_progress + curr / 3, file_size);
            });
        if (result != platform::ResultCode::SUCCESS) return result;
        current_progress += file_size / 3;

        // Pass 2: ones
        result = impl::overwrite_file(filepath, platform::ONE_PATTERN,
            [&](uint64_t curr, uint64_t total) {
                if (progress) progress(current_progress + curr / 3, file_size);
            });
        if (result != platform::ResultCode::SUCCESS) return result;
        current_progress += file_size / 3;

        // Pass 3: random
        result = impl::overwrite_file_random(filepath,
            [&](uint64_t curr, uint64_t total) {
                if (progress) progress(current_progress + curr / 3, file_size);
            });
        if (result != platform::ResultCode::SUCCESS) return result;
    }
    // Single pass methods
    else if (method == DeletionMethod::ZERO_PASS) {
        auto result = impl::overwrite_file(filepath, platform::ZERO_PATTERN, progress);
        if (result != platform::ResultCode::SUCCESS) return result;
    }
    else if (method == DeletionMethod::RANDOM_PASS) {
        auto result = impl::overwrite_file_random(filepath, progress);
        if (result != platform::ResultCode::SUCCESS) return result;
    }
    else {
        // For other methods (DOD 7-pass, Gutmann), implement as needed
        // For now, fallback to 3-pass
        return secure_delete_file(filepath, DeletionMethod::DOD_3_PASS, progress);
    }

    // Finally, delete the file
    return impl::delete_file(filepath);
}

platform::ResultCode secure_delete_directory(
    const std::string& dirpath,
    DeletionMethod method,
    ProgressCallback progress
) {
    if (!directory_exists(dirpath)) {
        return platform::ResultCode::ERROR_DEVICE_NOT_FOUND;
    }

    // Get all files
    auto files = list_files_recursive(dirpath);

    // Calculate total size
    uint64_t total_size = 0;
    for (const auto& file : files) {
        total_size += get_file_size(file);
    }

    uint64_t processed = 0;

    // Delete each file
    for (const auto& file : files) {
        uint64_t file_size = get_file_size(file);

        auto result = secure_delete_file(file, method,
            [&](uint64_t curr, uint64_t total) {
                if (progress) {
                    progress(processed + curr, total_size);
                }
            });

        if (result != platform::ResultCode::SUCCESS) {
            return result;
        }

        processed += file_size;
    }

    // Delete the directory itself
    return impl::delete_directory(dirpath);
}

} // namespace file_ops
} // namespace cleaner
