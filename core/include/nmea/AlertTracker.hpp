#pragma once

#include "nmea/SensorFrame.hpp"

#include <chrono>
#include <functional>
#include <string>
#include <unordered_map>
#include <vector>

namespace nmea {

/**
 * @brief Tracks alert state transitions per sensor channel.
 *
 * Fires callbacks only on genuine state changes:
 *   Normal → Warning/Critical : onAlertOpened
 *   Warning/Critical → Normal : onAlertClosed
 *
 * Uses uint8_t sensor ids — no hardcoded enums.
 */
class AlertTracker {
public:
    using TimePoint = std::chrono::steady_clock::time_point;

    struct AlertEvent {
        uint8_t    sensorId   {0xFF};
        std::string sensorName;          ///< Set from config at open time
        AlertLevel level      {AlertLevel::Normal};
        double     openValue  {0.0};
        double     closeValue {0.0};
        TimePoint  openTime;
        TimePoint  closeTime;
        std::string unit;

        [[nodiscard]] std::chrono::seconds duration() const {
            return std::chrono::duration_cast<std::chrono::seconds>(
                closeTime - openTime);
        }
    };

    using OpenCallback  = std::function<void(const AlertEvent&)>;
    using CloseCallback = std::function<void(const AlertEvent&)>;

    void setOnAlertOpened(OpenCallback  cb) { m_onOpened = std::move(cb); }
    void setOnAlertClosed(CloseCallback cb) { m_onClosed = std::move(cb); }

    void process(const SensorFrame& frame, const std::string& sensorName);

    [[nodiscard]] bool isOpen(uint8_t id) const;
    [[nodiscard]] std::vector<AlertEvent> openEvents() const;

private:
    struct ChannelState {
        AlertLevel  level  {AlertLevel::Normal};
        AlertEvent  current;
        bool        isOpen {false};
    };

    std::unordered_map<uint8_t, ChannelState> m_states;
    OpenCallback  m_onOpened;
    CloseCallback m_onClosed;
};

} // namespace nmea