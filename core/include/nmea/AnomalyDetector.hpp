#pragma once

#include "nmea/SensorFrame.hpp"
#include <functional>
#include <unordered_map>

namespace nmea {

/**
 * @brief Threshold configuration for a single sensor channel.
 */
struct SensorThreshold {
    double warningLow   {-1e9};
    double warningHigh  { 1e9};
    double criticalLow  {-1e9};
    double criticalHigh { 1e9};
};

/**
 * @brief Assigns an AlertLevel to incoming SensorFrames.
 *
 * Uses uint8_t sensor IDs loaded from sensors.json — no hardcoded enums.
 * Implements the Strategy pattern: evaluation algorithm is replaceable
 * at runtime via setStrategy().
 */
class AnomalyDetector {
public:
    using Strategy = std::function<AlertLevel(const SensorFrame&,
                                               const SensorThreshold&)>;

    explicit AnomalyDetector();

    void setThreshold(uint8_t id, SensorThreshold threshold);
    void setStrategy(Strategy strategy);

    void evaluate(SensorFrame& frame) const;

private:
    std::unordered_map<uint8_t, SensorThreshold> m_thresholds;
    Strategy                                      m_strategy;
};

} // namespace nmea