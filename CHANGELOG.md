# Changelog

All notable changes to this project are documented in this file.

The format follows [Keep a Changelog](https://keepachangelog.com/en/1.1.0/),
and this project follows [Semantic Versioning](https://semver.org/).

## [1.1.0] - 2026-08-17

### Added
- Single source of truth for the project version: `PROJECT_VERSION` in
  `CMakeLists.txt` is now injected into the binary via a generated
  `include/core/version.hpp` (from `version.hpp.in`), and `cleanerfirmware
  version` reports it instead of a hardcoded string.
- `.gitignore` — excludes `.claude/` and `historic/` (local/assistant-only
  folders, never pushed), plus standard CMake/C++ build artifacts.

### Fixed
- **Command injection** in `disk_ops_linux.cpp` (`unlock_disk_impl`,
  `format_disk_impl`) and `disk_ops_macos.cpp` (`format_disk_impl`):
  replaced `system()` calls built from concatenated shell strings with
  `execvp`-based argv execution (new `include/core/process_exec.hpp`).
- `cli.cpp`: removed a hardcoded `options.force = true`; `format` now only
  bypasses the mounted-disk safety guard when the user explicitly passes
  `--force`.
- Destructive `dd` write-tests in `diagnose_usb.sh` / `advanced_unlock.sh`
  now require explicit yes/no confirmation before writing, and all device
  path variables are quoted to remove word-splitting/injection risk.
- Missing `<fcntl.h>` include in `file_ops_macos.cpp` (`F_FULLFSYNC` build
  break on macOS).

See `historic/` for day-by-day session notes (not tracked in git).

## [1.0.0] - 2025-12-28

### Added
- Initial release: cross-platform (Linux/macOS/Windows) secure file and
  directory deletion (simple, zero-pass, random-pass, DoD 3/7-pass,
  Gutmann 35-pass), disk listing/info/format/unlock/wipe, and CLI.
