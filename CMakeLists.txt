cmake_minimum_required(VERSION 3.21)
project(nmea-monitor-cpp VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

find_package(Qt6 REQUIRED COMPONENTS Core Quick QuickControls2)
qt_standard_project_setup()

# ── Options ────────────────────────────────────────────────────────────────
option(NMEA_BUILD_TESTS "Build unit tests" ON)

# ── Sub-targets ────────────────────────────────────────────────────────────
add_subdirectory(core)
add_subdirectory(serial)
add_subdirectory(simulator)   # independent executable — no monitor dependency
add_subdirectory(monitor)     # independent executable — no simulator dependency

if(NMEA_BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()