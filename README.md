# CleanerFirmware

Cross-platform secure file deletion and disk management utility written in modern C++.

## Features

- **Secure File/Directory Deletion** with multiple methods:
  - Simple (standard deletion)
  - Zero pass (overwrite with zeros)
  - Random pass (overwrite with random data)
  - DoD 3-pass (DoD 5220.22-M standard)
  - DoD 7-pass
  - Gutmann 35-pass

- **Disk Management**:
  - List available disks
  - Detailed disk information
  - Format disks (ext4, NTFS, FAT32, exFAT, APFS, HFS+)
  - Unlock write-protected disks (multiple methods)
  - Secure disk wiping

- **Cross-platform**: Linux, Windows, macOS

## Prerequisites

### Linux
- GCC/Clang with C++20 support
- CMake >= 3.20
- libblkid-dev (for filesystem detection)

```bash
sudo apt-get install build-essential cmake libblkid-dev
```

### Windows
- Visual Studio 2019 or newer
- CMake >= 3.20

### macOS
- Xcode Command Line Tools
- CMake >= 3.20

```bash
brew install cmake
```

## Build

### Linux/macOS

```bash
mkdir build
cd build
cmake ..
make
```

### Windows (Visual Studio)

```bash
mkdir build
cd build
cmake .. -G "Visual Studio 16 2019"
cmake --build . --config Release
```

## Installation

```bash
sudo make install
```

Or simply copy the executable wherever you want.

## Usage

### Show help

```bash
./cleanerfirmware help
```

### Secure file deletion

```bash
# Simple deletion
./cleanerfirmware delete-file /path/to/file simple

# DoD 3-pass method (recommended)
./cleanerfirmware delete-file /path/to/file dod3

# Gutmann 35-pass method (very secure but slow)
./cleanerfirmware delete-file /path/to/file gutmann
```

### Secure directory deletion

```bash
./cleanerfirmware delete-dir /path/to/directory dod3
```

### Disk management

```bash
# List all disks
./cleanerfirmware list-disks

# Show disk information
./cleanerfirmware disk-info /dev/sdb1

# Unlock write-protected disk
sudo ./cleanerfirmware unlock /dev/sdb

# Format disk as ext4
sudo ./cleanerfirmware format /dev/sdb1 ext4

# Secure wipe with 3 passes
sudo ./cleanerfirmware wipe /dev/sdb 3
```

## Warnings

- **CAUTION**: Formatting and wiping operations are IRREVERSIBLE
- Disk operations require root/administrator privileges
- Always verify the target device before performing destructive operations
- Multi-pass methods (DoD 7-pass, Gutmann) can take considerable time on large volumes

## Project Structure

```
cleanerfirware/
├── include/
│   ├── core/
│   │   ├── platform.hpp           # Common definitions and types
│   │   ├── file_operations.hpp    # File deletion interface
│   │   └── disk_operations.hpp    # Disk management interface
│   └── cli/
│       └── cli.hpp                 # Command-line interface
├── src/
│   ├── core/
│   │   ├── file_operations.cpp    # Common implementation
│   │   ├── disk_operations.cpp    # Common implementation
│   │   └── platform/
│   │       ├── linux/              # Linux implementations
│   │       ├── windows/            # Windows implementations
│   │       └── macos/              # macOS implementations
│   └── cli/
│       └── cli.cpp                 # CLI implementation
├── main.cpp
└── CMakeLists.txt
```

## Unlock Utilities

For stubborn write-protected USB drives, additional diagnostic tools are provided:

- **diagnose_usb.sh** - Basic diagnostic and unlock attempts
- **advanced_unlock.sh** - Advanced low-level unlock methods (SCSI, USB reset, etc.)
- **identify_controller.sh** - Identify USB controller type

```bash
sudo ./diagnose_usb.sh /dev/sdX
sudo ./advanced_unlock.sh /dev/sdX
./identify_controller.sh /dev/sdX
```

## Security

This software performs destructive operations on data. While secure deletion methods are designed to make data recovery extremely difficult, no absolute guarantee can be provided.

For highly sensitive data, consider:
- Using multiple passes (3-7)
- Physical destruction of media
- Prior encryption of data

## Current Limitations

- DoD 7-pass and Gutmann 35-pass currently use DoD 3-pass implementation
- No support for encrypted volumes
- No post-write verification
- Windows and macOS implementations are partial (stubs)
