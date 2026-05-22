add_library(nmea_serial STATIC
    src/SerialPortReader.cpp
)

target_include_directories(nmea_serial
    PUBLIC  ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_compile_options(nmea_serial PRIVATE
    -Wall -Wextra -Wpedantic
)

target_link_libraries(nmea_serial PUBLIC nmea_core)