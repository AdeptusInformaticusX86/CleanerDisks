#include "cli/cli.hpp"
#include "core/file_operations.hpp"
#include "core/disk_operations.hpp"
#include <iostream>
#include <iomanip>
#include <algorithm>

namespace cleaner {
namespace cli {

CLI::CLI(int argc, char* argv[]) : argc_(argc) {
    for (int i = 0; i < argc; ++i) {
        argv_.emplace_back(argv[i]);
    }
}

void CLI::print_help() {
    std::cout << "CleanerFirmware - Secure file deletion and disk management tool\n\n";
    std::cout << "Usage: cleanerfirmware [command] [options]\n\n";
    std::cout << "Commands:\n";
    std::cout << "  delete-file <path> [method]     - Securely delete a file\n";
    std::cout << "  delete-dir <path> [method]      - Securely delete a directory\n";
    std::cout << "  list-disks                      - List all available disks\n";
    std::cout << "  disk-info <device>              - Show disk information\n";
    std::cout << "  format <device> [filesystem] [--force] - Format a disk\n";
    std::cout << "                                     (--force required if the disk is mounted)\n";
    std::cout << "  unlock <device>                 - Unlock write-protected disk\n";
    std::cout << "  wipe <device> [passes]          - Securely wipe a disk\n";
    std::cout << "  help                            - Show this help\n";
    std::cout << "  version                         - Show version\n\n";
    std::cout << "Deletion methods:\n";
    std::cout << "  simple     - Simple deletion (default)\n";
    std::cout << "  zero       - Single pass with zeros\n";
    std::cout << "  random     - Single pass with random data\n";
    std::cout << "  dod3       - DoD 3-pass method\n";
    std::cout << "  dod7       - DoD 7-pass method\n";
    std::cout << "  gutmann    - Gutmann 35-pass method\n\n";
    std::cout << "Filesystems:\n";
    std::cout << "  ext4, ntfs, fat32, exfat, apfs, hfsplus\n\n";
    std::cout << "Examples:\n";
    std::cout << "  cleanerfirmware delete-file /path/to/file dod3\n";
    std::cout << "  cleanerfirmware format /dev/sdb1 ext4\n";
    std::cout << "  cleanerfirmware wipe /dev/sdc 3\n";
}

void CLI::print_version() {
    std::cout << "CleanerFirmware version 1.0.0\n";
    std::cout << "Built for platform: ";
#ifdef PLATFORM_WINDOWS
    std::cout << "Windows\n";
#elif defined(PLATFORM_MACOS)
    std::cout << "macOS\n";
#elif defined(PLATFORM_LINUX)
    std::cout << "Linux\n";
#endif
}

void CLI::progress_callback(uint64_t current, uint64_t total) {
    if (total == 0) return;

    int percent = static_cast<int>((current * 100) / total);
    int bar_width = 50;
    int pos = (bar_width * current) / total;

    std::cout << "\r[";
    for (int i = 0; i < bar_width; ++i) {
        if (i < pos) std::cout << "=";
        else if (i == pos) std::cout << ">";
        else std::cout << " ";
    }
    std::cout << "] " << percent << "% ";
    std::cout.flush();
}

int CLI::handle_delete_file() {
    if (argv_.size() < 3) {
        std::cerr << "Error: Missing file path\n";
        return 1;
    }

    std::string filepath = argv_[2];
    file_ops::DeletionMethod method = file_ops::DeletionMethod::SIMPLE;

    if (argv_.size() >= 4) {
        std::string method_str = argv_[3];
        if (method_str == "zero") method = file_ops::DeletionMethod::ZERO_PASS;
        else if (method_str == "random") method = file_ops::DeletionMethod::RANDOM_PASS;
        else if (method_str == "dod3") method = file_ops::DeletionMethod::DOD_3_PASS;
        else if (method_str == "dod7") method = file_ops::DeletionMethod::DOD_7_PASS;
        else if (method_str == "gutmann") method = file_ops::DeletionMethod::GUTMANN_35_PASS;
    }

    std::cout << "Deleting file: " << filepath << "\n";
    auto result = file_ops::secure_delete_file(filepath, method, progress_callback);

    std::cout << "\n";
    if (result == platform::ResultCode::SUCCESS) {
        std::cout << "File deleted successfully\n";
        return 0;
    } else {
        std::cerr << "Error: " << platform::result_code_to_string(result) << "\n";
        return 1;
    }
}

int CLI::handle_delete_directory() {
    if (argv_.size() < 3) {
        std::cerr << "Error: Missing directory path\n";
        return 1;
    }

    std::string dirpath = argv_[2];
    file_ops::DeletionMethod method = file_ops::DeletionMethod::DOD_3_PASS;

    if (argv_.size() >= 4) {
        std::string method_str = argv_[3];
        if (method_str == "simple") method = file_ops::DeletionMethod::SIMPLE;
        else if (method_str == "zero") method = file_ops::DeletionMethod::ZERO_PASS;
        else if (method_str == "random") method = file_ops::DeletionMethod::RANDOM_PASS;
        else if (method_str == "dod3") method = file_ops::DeletionMethod::DOD_3_PASS;
        else if (method_str == "dod7") method = file_ops::DeletionMethod::DOD_7_PASS;
        else if (method_str == "gutmann") method = file_ops::DeletionMethod::GUTMANN_35_PASS;
    }

    std::cout << "Deleting directory: " << dirpath << "\n";
    auto result = file_ops::secure_delete_directory(dirpath, method, progress_callback);

    std::cout << "\n";
    if (result == platform::ResultCode::SUCCESS) {
        std::cout << "Directory deleted successfully\n";
        return 0;
    } else {
        std::cerr << "Error: " << platform::result_code_to_string(result) << "\n";
        return 1;
    }
}

int CLI::handle_list_disks() {
    auto disks = disk_ops::list_disks();

    if (disks.empty()) {
        std::cout << "No disks found\n";
        return 0;
    }

    std::cout << "\nAvailable disks:\n";
    std::cout << std::string(80, '-') << "\n";

    for (const auto& disk : disks) {
        std::cout << "Device: " << disk.device_path << "\n";
        std::cout << "  Mount: " << (disk.mount_point.empty() ? "(not mounted)" : disk.mount_point) << "\n";
        std::cout << "  Filesystem: " << disk.filesystem << "\n";
        std::cout << "  Size: " << (disk.total_size / (1024*1024*1024)) << " GB\n";
        std::cout << "  Removable: " << (disk.is_removable ? "Yes" : "No") << "\n";
        std::cout << "  Read-only: " << (disk.is_read_only ? "Yes" : "No") << "\n";
        std::cout << std::string(80, '-') << "\n";
    }

    return 0;
}

int CLI::handle_disk_info() {
    if (argv_.size() < 3) {
        std::cerr << "Error: Missing device path\n";
        return 1;
    }

    std::string device = argv_[2];
    platform::DiskInfo info;

    auto result = disk_ops::get_disk_info(device, info);
    if (result != platform::ResultCode::SUCCESS) {
        std::cerr << "Error: " << platform::result_code_to_string(result) << "\n";
        return 1;
    }

    std::cout << "\nDisk information:\n";
    std::cout << "  Device: " << info.device_path << "\n";
    std::cout << "  Mount: " << (info.mount_point.empty() ? "(not mounted)" : info.mount_point) << "\n";
    std::cout << "  Filesystem: " << info.filesystem << "\n";
    std::cout << "  Total size: " << (info.total_size / (1024*1024*1024)) << " GB\n";
    std::cout << "  Free size: " << (info.free_size / (1024*1024*1024)) << " GB\n";
    std::cout << "  Removable: " << (info.is_removable ? "Yes" : "No") << "\n";
    std::cout << "  Read-only: " << (info.is_read_only ? "Yes" : "No") << "\n";

    return 0;
}

int CLI::handle_format_disk() {
    if (argv_.size() < 3) {
        std::cerr << "Error: Missing device path\n";
        return 1;
    }

    std::string device = argv_[2];
    disk_ops::FormatOptions options;

    for (size_t i = 3; i < argv_.size(); ++i) {
        const std::string& arg = argv_[i];
        if (arg == "--force") options.force = true;
        else if (arg == "ext4") options.filesystem = disk_ops::FilesystemType::EXT4;
        else if (arg == "ntfs") options.filesystem = disk_ops::FilesystemType::NTFS;
        else if (arg == "fat32") options.filesystem = disk_ops::FilesystemType::FAT32;
        else if (arg == "exfat") options.filesystem = disk_ops::FilesystemType::EXFAT;
        else if (arg == "apfs") options.filesystem = disk_ops::FilesystemType::APFS;
        else if (arg == "hfsplus") options.filesystem = disk_ops::FilesystemType::HFS_PLUS;
    }

    std::cout << "WARNING: This will erase all data on " << device << "\n";
    std::cout << "Are you sure? (yes/no): ";
    std::string confirm;
    std::cin >> confirm;

    if (confirm != "yes") {
        std::cout << "Operation cancelled\n";
        return 0;
    }

    std::cout << "Formatting disk...\n";
    auto result = disk_ops::format_disk(device, options, progress_callback);

    std::cout << "\n";
    if (result == platform::ResultCode::SUCCESS) {
        std::cout << "Disk formatted successfully\n";
        return 0;
    } else {
        std::cerr << "Error: " << platform::result_code_to_string(result) << "\n";
        return 1;
    }
}

int CLI::handle_unlock_disk() {
    if (argv_.size() < 3) {
        std::cerr << "Error: Missing device path\n";
        return 1;
    }

    std::string device = argv_[2];
    std::cout << "Unlocking disk: " << device << "\n";

    auto result = disk_ops::unlock_disk(device);
    if (result == platform::ResultCode::SUCCESS) {
        std::cout << "Disk unlocked successfully\n";
        return 0;
    } else {
        std::cerr << "Error: " << platform::result_code_to_string(result) << "\n";
        return 1;
    }
}

int CLI::handle_wipe_disk() {
    if (argv_.size() < 3) {
        std::cerr << "Error: Missing device path\n";
        return 1;
    }

    std::string device = argv_[2];
    uint32_t passes = 1;

    if (argv_.size() >= 4) {
        try {
            passes = std::stoi(argv_[3]);
        } catch(...) {
            std::cerr << "Error: Invalid number of passes\n";
            return 1;
        }
    }

    std::cout << "WARNING: This will securely wipe all data on " << device << "\n";
    std::cout << "Number of passes: " << passes << "\n";
    std::cout << "Are you sure? (yes/no): ";
    std::string confirm;
    std::cin >> confirm;

    if (confirm != "yes") {
        std::cout << "Operation cancelled\n";
        return 0;
    }

    std::cout << "Wiping disk...\n";
    auto result = disk_ops::secure_wipe_disk(device, passes, progress_callback);

    std::cout << "\n";
    if (result == platform::ResultCode::SUCCESS) {
        std::cout << "Disk wiped successfully\n";
        return 0;
    } else {
        std::cerr << "Error: " << platform::result_code_to_string(result) << "\n";
        return 1;
    }
}

int CLI::run() {
    if (argc_ < 2) {
        print_help();
        return 0;
    }

    std::string command = argv_[1];

    if (command == "help" || command == "--help" || command == "-h") {
        print_help();
        return 0;
    }
    else if (command == "version" || command == "--version" || command == "-v") {
        print_version();
        return 0;
    }
    else if (command == "delete-file") {
        return handle_delete_file();
    }
    else if (command == "delete-dir") {
        return handle_delete_directory();
    }
    else if (command == "list-disks") {
        return handle_list_disks();
    }
    else if (command == "disk-info") {
        return handle_disk_info();
    }
    else if (command == "format") {
        return handle_format_disk();
    }
    else if (command == "unlock") {
        return handle_unlock_disk();
    }
    else if (command == "wipe") {
        return handle_wipe_disk();
    }
    else {
        std::cerr << "Error: Unknown command '" << command << "'\n";
        std::cerr << "Use 'cleanerfirmware help' for usage information\n";
        return 1;
    }
}

} // namespace cli
} // namespace cleaner
