# CleanerFirmware GUI

Qt-based graphical user interface for CleanerFirmware.

## Features

- **Secure File Deletion Tab**
  - Browse and select files/directories
  - Queue multiple items for deletion
  - Choose deletion method (Simple, DoD, Gutmann, etc.)
  - Batch processing with progress tracking

- **Disk Management Tab**
  - List all available disks
  - View disk information (size, filesystem, read-only status)
  - Unlock write-protected USB drives
  - Format disks with various filesystems
  - Secure disk wiping

## Prerequisites

### Linux
```bash
# Debian/Ubuntu
sudo apt-get install qt6-base-dev qt6-tools-dev
# Or for Qt5
sudo apt-get install qtbase5-dev qttools5-dev

# Fedora/RHEL
sudo dnf install qt6-qtbase-devel
# Or for Qt5
sudo dnf install qt5-qtbase-devel

# Arch
sudo pacman -S qt6-base
# Or for Qt5
sudo pacman -S qt5-base
```

### Windows
- Install Qt from https://www.qt.io/download
- Add Qt to CMake path

### macOS
```bash
brew install qt@6
# Or
brew install qt@5
```

## Building

### Build GUI along with CLI

```bash
cd build
cmake .. -DBUILD_GUI=ON
make
```

This will create two executables:
- `cleanerfirmware` - CLI version
- `cleanerfirmware-gui` - GUI version

### Build only GUI

```bash
cd build
cmake .. -DBUILD_GUI=ON
make cleanerfirmware-gui
```

### Specifying Qt version

If you have both Qt5 and Qt6 installed:

```bash
# For Qt6
cmake .. -DBUILD_GUI=ON -DCMAKE_PREFIX_PATH=/usr/lib/qt6

# For Qt5
cmake .. -DBUILD_GUI=ON -DCMAKE_PREFIX_PATH=/usr/lib/qt5
```

## Running

### Linux
```bash
# Most disk operations require root
sudo ./cleanerfirmware-gui

# For file deletion only (no root needed)
./cleanerfirmware-gui
```

### Windows
Run as Administrator for disk operations.

### macOS
```bash
sudo ./cleanerfirmware-gui
```

## Architecture

The GUI is built on top of the core library:

```
cleanerfirmware-gui
    ↓
cleanerfirmware-core (static library)
    ↓
Platform-specific implementations (Linux/Windows/macOS)
```

## Widgets

- **MainWindow** - Main application window with tabs
- **SecureDeleteWidget** - File/directory secure deletion interface
- **DiskWidget** - Disk management interface

## Future Enhancements

- [ ] Progress dialog for long operations
- [ ] Settings/preferences dialog
- [ ] File browser with preview
- [ ] Detailed operation logs
- [ ] Disk usage visualization
- [ ] Scheduled deletion tasks
- [ ] Drag and drop support
- [ ] System tray integration
