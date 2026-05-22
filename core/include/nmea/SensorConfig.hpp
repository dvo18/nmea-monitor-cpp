#pragma once

#include "nmea/AnomalyDetector.hpp"

#include <string>
#include <unordered_map>

namespace nmea {

/**
 * @brief Connection configuration loaded from sensors.json.
 */
struct ConnectionConfig {
    std::string port     {"/dev/ttyUSB0"};
    int         baudRate {4800};
    std::string notes;
};

/**
 * @brief Per-sensor configuration loaded from sensors.json.
 *
 * nmeaSentence and nmeaField drive the generic TelemetryParser —
 * no sensor-specific code exists in the parser itself.
 *
 * nmeaField is the 0-based index of the numeric value within the
 * comma-separated fields after the sentence type identifier.
 *
 * Example: "$IIRPM,E,1,1450.0,45.0,A"
 *   fields: [E, 1, 1450.0, 45.0, A]
 *   nmeaField=2 → 1450.0
 *
 * Example: "$IIMTW,82.5,C"
 *   fields: [82.5, C]
 *   nmeaField=0 → 82.5
 */
struct SensorConfig {
    uint8_t     id           {0xFF};
    std::string name         {};
    std::string nmeaSentence {};  ///< e.g. "IIRPM", "IIMTW", "PSHIP"
    int         nmeaField    {0}; ///< 0-based field index to extract
    std::string unit         {};
    int         updateHz     {1};
    SensorThreshold threshold;
    std::string notes        {};
};

/**
 * @brief Result of loading sensors.json.
 */
struct LoadedConfig {
    ConnectionConfig                          connection;
    std::unordered_map<uint8_t, SensorConfig> sensors;
};

/**
 * @brief Loads sensor and connection configuration from sensors.json.
 *
 * Populates the AnomalyDetector with thresholds from the file.
 * Returns a LoadedConfig containing all sensor metadata.
 */
class SensorConfigLoader {
public:
    static LoadedConfig load(const std::string& path, AnomalyDetector& detector);
};

} // namespace nmea