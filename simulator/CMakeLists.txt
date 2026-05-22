# ── Simulator library ──────────────────────────────────────────────────────
add_library(nmea_simulator STATIC
    src/SensorSimulator.cpp
)

target_include_directories(nmea_simulator
    PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_compile_options(nmea_simulator PRIVATE
    -Wall -Wextra -Wpedantic
)

target_link_libraries(nmea_simulator PUBLIC nmea_core)

# ── Simulator executable ───────────────────────────────────────────────────
# Completely independent program. Opens a POSIX pty, prints the slave
# path to stdout, and writes NMEA sentences to the master fd.
# The monitor reads from the slave path — identical to /dev/ttyUSB0.
add_executable(nmea_simulator_bin src/main.cpp)

set_target_properties(nmea_simulator_bin PROPERTIES
    OUTPUT_NAME "nmea_simulator"
)

target_link_libraries(nmea_simulator_bin PRIVATE nmea_simulator)