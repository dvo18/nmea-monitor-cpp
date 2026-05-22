#pragma once

#include <chrono>
#include <cstdint>
#include <string>

namespace nmea {

/// Severity level assigned by the AnomalyDetector.
enum class AlertLevel : uint8_t {
    Normal   = 0,
    Warning  = 1,
    Critical = 2,
};

/**
 * @brief A single telemetry frame produced by a sensor.
 *
 * sensorId is a uint8_t loaded from sensors.json — not a hardcoded enum.
 * The human-readable name comes from SensorConfig, not from this struct.
 *
 * All timestamps use steady_clock to remain monotonic regardless of
 * system-clock adjustments (important on embedded platforms).
 */
struct SensorFrame {
    uint8_t    sensorId   {0xFF};
    double     value      {0.0};
    std::string unit      {};
    AlertLevel alertLevel {AlertLevel::Normal};

    std::chrono::steady_clock::time_point timestamp
        {std::chrono::steady_clock::now()};
};

} // namespace nmea