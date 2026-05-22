qt_add_executable(nmea_monitor
    src/main.cpp
    src/SensorModel.cpp
    src/SensorModel.hpp
)

qt_add_resources(nmea_monitor "qml_resources"
    PREFIX "/"
    FILES
        qml/Main.qml
)

target_include_directories(nmea_monitor PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)

# The monitor only knows about core and serial.
# It has zero knowledge of the simulator — they are separate executables
# that communicate exclusively through a serial port.
target_link_libraries(nmea_monitor PRIVATE
    nmea_core
    nmea_serial
    Qt6::Core
    Qt6::Quick
    Qt6::QuickControls2
)

set_target_properties(nmea_monitor PROPERTIES
    WIN32_EXECUTABLE TRUE
    MACOSX_BUNDLE    TRUE
)