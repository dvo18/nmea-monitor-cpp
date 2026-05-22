#include "nmea/AnomalyDetector.hpp"

namespace nmea {

static AlertLevel defaultStrategy(const SensorFrame& frame,
                                   const SensorThreshold& th)
{
    const double v = frame.value;
    if (v <= th.criticalLow || v >= th.criticalHigh) return AlertLevel::Critical;
    if (v <= th.warningLow  || v >= th.warningHigh)  return AlertLevel::Warning;
    return AlertLevel::Normal;
}

AnomalyDetector::AnomalyDetector()
    : m_strategy{defaultStrategy}
{}

void AnomalyDetector::setThreshold(uint8_t id, SensorThreshold threshold)
{
    m_thresholds[id] = std::move(threshold);
}

void AnomalyDetector::setStrategy(Strategy strategy)
{
    m_strategy = std::move(strategy);
}

void AnomalyDetector::evaluate(SensorFrame& frame) const
{
    auto it = m_thresholds.find(frame.sensorId);
    if (it == m_thresholds.end()) {
        frame.alertLevel = AlertLevel::Normal;
        return;
    }
    frame.alertLevel = m_strategy(frame, it->second);
}

} // namespace nmea