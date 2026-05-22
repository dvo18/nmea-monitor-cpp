#pragma once

#include "nmea/AlertTracker.hpp"

#include <fstream>
#include <mutex>
#include <string>

namespace nmea {

/**
 * @brief Writes alert events to a per-session log file.
 *
 * File name: logs/session_YYYY-MM-DD_HH-MM-SS.log
 * Created when the monitor starts. Each line is flushed immediately
 * so that if the process crashes, all closed events and any open
 * events at shutdown time are preserved.
 *
 * Log format:
 *   [2026-05-21T22:14:03] OPEN   WARNING  Engine RPM    1923.40 RPM  threshold: >= 1900.0
 *   [2026-05-21T22:14:18] CLOSED WARNING  Engine RPM    1450.20 RPM  duration: 00:00:15
 *
 * Salvaguarda: on shutdown (or SIGTERM), flushOpenEvents() is called
 * to write all still-open alerts with their partial duration and an
 * INTERRUPTED marker so they are not silently lost.
 */
class SessionLogger {
public:
    explicit SessionLogger(const std::string& logDir = "logs");
    ~SessionLogger();

    SessionLogger(const SessionLogger&)            = delete;
    SessionLogger& operator=(const SessionLogger&) = delete;

    /// Write an OPEN entry. Called when an alert first fires.
    void logOpen(const AlertTracker::AlertEvent& event,
                 const std::string& thresholdDesc);

    /// Write a CLOSED entry with duration. Called when alert resolves.
    void logClose(const AlertTracker::AlertEvent& event);

    /// Write INTERRUPTED entries for all still-open alerts.
    /// Call before shutdown.
    void flushOpenEvents(const std::vector<AlertTracker::AlertEvent>& openEvents);

    [[nodiscard]] const std::string& filePath() const noexcept { return m_filePath; }

private:
    void writeLine(const std::string& line);
    static std::string sessionFileName(const std::string& dir);
    static std::string nowIso();
    static std::string formatDuration(std::chrono::seconds s);
    static std::string levelStr(AlertLevel level);

    std::ofstream m_file;
    std::string   m_filePath;
    std::mutex    m_mutex;
};

} // namespace nmea