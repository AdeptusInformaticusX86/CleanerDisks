cmake_minimum_required(VERSION 3.20)

# Qt specific settings
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

# Find Qt6 (fallback to Qt5 if not found)
find_package(Qt6 COMPONENTS Widgets QUIET)
if(NOT Qt6_FOUND)
    find_package(Qt5 COMPONENTS Widgets REQUIRED)
    set(QT_VERSION_MAJOR 5)
else()
    set(QT_VERSION_MAJOR 6)
endif()

message(STATUS "Using Qt${QT_VERSION_MAJOR}")

# GUI source files
set(GUI_SOURCES
    main.cpp
    mainwindow.cpp
    mainwindow.h
    widgets/disk_widget.cpp
    widgets/disk_widget.h
    widgets/secure_delete_widget.cpp
    widgets/secure_delete_widget.h
)

# Create GUI executable
add_executable(cleanerfirmware-gui ${GUI_SOURCES})

# Link Qt libraries
if(QT_VERSION_MAJOR EQUAL 6)
    target_link_libraries(cleanerfirmware-gui PRIVATE Qt6::Widgets)
else()
    target_link_libraries(cleanerfirmware-gui PRIVATE Qt5::Widgets)
endif()

# Link with core library (we'll create this)
target_link_libraries(cleanerfirmware-gui PRIVATE cleanerfirmware-core)

# Include directories
target_include_directories(cleanerfirmware-gui PRIVATE
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}
)

# Install
install(TARGETS cleanerfirmware-gui DESTINATION bin)
