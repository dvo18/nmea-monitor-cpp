#include "nmea/SessionLogger.hpp"

#include <chrono>
#include <ctime>
#include <filesystem>
#include <iomanip>
#include <iostream>
#include <sstream>

namespace nmea {

SessionLogger::SessionLogger(const std::string& logDir)
{
    const char* envDir = std::getenv("NMEA_LOG_DIR");
    const std::string dir = (envDir && envDir[0] != '\0') ? envDir : logDir;

    std::filesystem::create_directories(dir);
    m_filePath = sessionFileName(dir);
    m_file.open(m_filePath, std::ios::out | std::ios::app);

    if (!m_file.is_open()) {
        std::cerr << "[SessionLogger] cannot open " << m_filePath << "\n";
        return;
    }

    writeLine("# nmea-monitor-cpp session log");
    writeLine("# started: " + nowIso());
    writeLine("# format: [timestamp] OPEN|CLOSED|INTERRUPTED level sensor value unit [duration]");
    writeLine("#");
    std::cout << "[SessionLogger] logging to " << m_filePath << "\n";
}

SessionLogger::~SessionLogger()
{
    if (m_file.is_open()) {
        writeLine("# session ended: " + nowIso());
        m_file.close();
    }
}

void SessionLogger::logOpen(const AlertTracker::AlertEvent& event,
                             const std::string& thresholdDesc)
{
    std::ostringstream ss;
    ss << "[" << nowIso() << "] "
       << "OPEN        "
       << std::left << std::setw(10) << levelStr(event.level)
       << std::setw(22) << event.sensorName
       << std::fixed << std::setprecision(2)
       << std::setw(10) << event.openValue
       << "  " << event.unit
       << "  threshold: " << thresholdDesc;
    writeLine(ss.str());
}

void SessionLogger::logClose(const AlertTracker::AlertEvent& event)
{
    const auto dur = event.duration();
    std::ostringstream ss;
    ss << "[" << nowIso() << "] "
       << "CLOSED      "
       << std::left << std::setw(10) << levelStr(event.level)
       << std::setw(22) << event.sensorName
       << std::fixed << std::setprecision(2)
       << std::setw(10) << event.closeValue
       << "  " << event.unit
       << "  duration: " << formatDuration(dur);
    writeLine(ss.str());
}

void SessionLogger::flushOpenEvents(
    const std::vector<AlertTracker::AlertEvent>& openEvents)
{
    if (openEvents.empty()) return;

    writeLine("# --- shutdown salvaguarda: " +
              std::to_string(openEvents.size()) +
              " alert(s) still open ---");

    for (const auto& ev : openEvents) {
        const auto partial = std::chrono::duration_cast<std::chrono::seconds>(
            std::chrono::steady_clock::now() - ev.openTime);

        std::ostringstream ss;
        ss << "[" << nowIso() << "] "
           << "INTERRUPTED "
           << std::left << std::setw(10) << levelStr(ev.level)
           << std::setw(22) << ev.sensorName
           << std::fixed << std::setprecision(2)
           << std::setw(10) << ev.openValue
           << "  " << ev.unit
           << "  partial duration: " << formatDuration(partial);
        writeLine(ss.str());
    }
}

void SessionLogger::writeLine(const std::string& line)
{
    std::lock_guard lock{m_mutex};
    if (m_file.is_open()) {
        m_file << line << "\n";
        m_file.flush();
    }
}

std::string SessionLogger::sessionFileName(const std::string& dir)
{
    const auto now = std::chrono::system_clock::now();
    const std::time_t t = std::chrono::system_clock::to_time_t(now);
    std::tm tm{};
    localtime_r(&t, &tm);
    char buf[64];
    std::strftime(buf, sizeof(buf), "session_%Y-%m-%d_%H-%M-%S.log", &tm);
    return dir + "/" + buf;
}

std::string SessionLogger::nowIso()
{
    const auto now = std::chrono::system_clock::now();
    const std::time_t t = std::chrono::system_clock::to_time_t(now);
    std::tm tm{};
    localtime_r(&t, &tm);
    char buf[32];
    std::strftime(buf, sizeof(buf), "%Y-%m-%dT%H:%M:%S", &tm);
    return buf;
}

std::string SessionLogger::formatDuration(std::chrono::seconds s)
{
    const long total = s.count();
    const int h   = static_cast<int>(total / 3600);
    const int m   = static_cast<int>((total % 3600) / 60);
    const int sec = static_cast<int>(total % 60);
    std::ostringstream ss;
    ss << std::setw(2) << std::setfill('0') << h << ":"
       << std::setw(2) << std::setfill('0') << m << ":"
       << std::setw(2) << std::setfill('0') << sec;
    return ss.str();
}

std::string SessionLogger::levelStr(AlertLevel level)
{
    switch (level) {
        case AlertLevel::Warning:  return "WARNING";
        case AlertLevel::Critical: return "CRITICAL";
        default:                   return "NORMAL";
    }
}

} // namespace nmea