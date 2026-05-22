#include "nmea/AlertTracker.hpp"

namespace nmea {

void AlertTracker::process(const SensorFrame& frame,
                             const std::string& sensorName)
{
    auto& state = m_states[frame.sensorId];

    const AlertLevel prev = state.level;
    const AlertLevel curr = frame.alertLevel;

    if (prev == AlertLevel::Normal && curr != AlertLevel::Normal) {
        // Alert opened
        state.isOpen             = true;
        state.level              = curr;
        state.current.sensorId   = frame.sensorId;
        state.current.sensorName = sensorName;
        state.current.level      = curr;
        state.current.openValue  = frame.value;
        state.current.openTime   = std::chrono::steady_clock::now();
        state.current.unit       = frame.unit;
        if (m_onOpened) m_onOpened(state.current);

    } else if (prev != AlertLevel::Normal && curr == AlertLevel::Normal) {
        // Alert closed
        state.current.closeValue = frame.value;
        state.current.closeTime  = std::chrono::steady_clock::now();
        state.isOpen             = false;
        state.level              = AlertLevel::Normal;
        if (m_onClosed) m_onClosed(state.current);

    } else if (prev != AlertLevel::Normal && curr != AlertLevel::Normal
               && prev != curr) {
        // Escalation — close old level, open new
        AlertEvent escalated     = state.current;
        escalated.closeValue     = frame.value;
        escalated.closeTime      = std::chrono::steady_clock::now();
        if (m_onClosed) m_onClosed(escalated);

        state.current.level      = curr;
        state.current.openValue  = frame.value;
        state.current.openTime   = std::chrono::steady_clock::now();
        state.level              = curr;
        if (m_onOpened) m_onOpened(state.current);
    }
}

bool AlertTracker::isOpen(uint8_t id) const
{
    auto it = m_states.find(id);
    return it != m_states.end() && it->second.isOpen;
}

std::vector<AlertTracker::AlertEvent> AlertTracker::openEvents() const
{
    std::vector<AlertEvent> result;
    for (const auto& [id, state] : m_states)
        if (state.isOpen) result.push_back(state.current);
    return result;
}

} // namespace nmea