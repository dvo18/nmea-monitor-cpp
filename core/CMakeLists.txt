add_library(nmea_core STATIC
    src/TelemetryParser.cpp
    src/AnomalyDetector.cpp
    src/SensorHub.cpp
    src/SensorConfig.cpp
    src/AlertTracker.cpp
    src/SessionLogger.cpp
)

target_include_directories(nmea_core
    PUBLIC  ${CMAKE_CURRENT_SOURCE_DIR}/include
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_compile_options(nmea_core PRIVATE
    -Wall -Wextra -Wpedantic -Werror
)

target_link_libraries(nmea_core PUBLIC Qt6::Core)