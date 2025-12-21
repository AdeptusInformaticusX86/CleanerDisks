#pragma once

#include <string>
#include <vector>
#include <cstdint>

namespace cleaner {
namespace cli {

// Command-line interface
class CLI {
public:
    CLI(int argc, char* argv[]);

    // Parse arguments and execute command
    int run();

private:
    void print_help();
    void print_version();

    // Command handlers
    int handle_delete_file();
    int handle_delete_directory();
    int handle_list_disks();
    int handle_disk_info();
    int handle_format_disk();
    int handle_unlock_disk();
    int handle_wipe_disk();

    // Progress callback
    static void progress_callback(uint64_t current, uint64_t total);

    int argc_;
    std::vector<std::string> argv_;
};

} // namespace cli
} // namespace cleaner
