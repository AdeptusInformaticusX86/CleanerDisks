cmake_minimum_required(VERSION 3.20)
project(CleanerFirmware VERSION 1.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Options
option(BUILD_TESTS "Build tests" OFF)

# Detect platform
if(WIN32)
    set(PLATFORM_NAME "windows")
elseif(APPLE)
    set(PLATFORM_NAME "macos")
elseif(UNIX)
    set(PLATFORM_NAME "linux")
endif()

message(STATUS "Building for platform: ${PLATFORM_NAME}")

# Generate version header from project version (single source of truth)
configure_file(
    ${CMAKE_SOURCE_DIR}/include/core/version.hpp.in
    ${CMAKE_BINARY_DIR}/generated/core/version.hpp
    @ONLY
)

# Include directories
include_directories(
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_BINARY_DIR}/generated
)

# Source files common to all platforms
set(COMMON_SOURCES
    src/core/file_operations.cpp
    src/core/disk_operations.cpp
    src/cli/cli.cpp
    main.cpp
)

# Platform-specific sources
if(WIN32)
    list(APPEND PLATFORM_SOURCES
        src/core/platform/windows/file_ops_win.cpp
        src/core/platform/windows/disk_ops_win.cpp
    )
elseif(APPLE)
    list(APPEND PLATFORM_SOURCES
        src/core/platform/macos/file_ops_macos.cpp
        src/core/platform/macos/disk_ops_macos.cpp
    )
elseif(UNIX)
    list(APPEND PLATFORM_SOURCES
        src/core/platform/linux/file_ops_linux.cpp
        src/core/platform/linux/disk_ops_linux.cpp
    )
endif()

# Create executable
add_executable(cleanerfirmware ${COMMON_SOURCES} ${PLATFORM_SOURCES})

# Platform-specific libraries
if(WIN32)
    target_compile_definitions(cleanerfirmware PRIVATE PLATFORM_WINDOWS)
    # Link Windows libraries
    target_link_libraries(cleanerfirmware PRIVATE)
elseif(APPLE)
    target_compile_definitions(cleanerfirmware PRIVATE PLATFORM_MACOS)
    # Link macOS frameworks
    find_library(CORE_FOUNDATION CoreFoundation)
    find_library(DISK_ARBITRATION DiskArbitration)
    find_library(IO_KIT IOKit)
    target_link_libraries(cleanerfirmware PRIVATE
        ${CORE_FOUNDATION}
        ${DISK_ARBITRATION}
        ${IO_KIT}
    )
elseif(UNIX)
    target_compile_definitions(cleanerfirmware PRIVATE PLATFORM_LINUX)
    # Link Linux libraries
    target_link_libraries(cleanerfirmware PRIVATE
        stdc++fs
        pthread
        blkid
    )
endif()

# Compiler warnings
if(MSVC)
    target_compile_options(cleanerfirmware PRIVATE /W4)
else()
    target_compile_options(cleanerfirmware PRIVATE -Wall -Wextra -Wpedantic)
endif()

# Install
install(TARGETS cleanerfirmware DESTINATION bin)
