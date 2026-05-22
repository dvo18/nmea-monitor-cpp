#pragma once

#include "nmea/AnomalyDetector.hpp"
#include "nmea/DataBuffer.hpp"
#include "nmea/SensorConfig.hpp"
#include "nmea/SensorFrame.hpp"
#include "nmea/TelemetryParser.hpp"

#include <atomic>
#include <functional>
#include <string>
#include <thread>

namespace nmea {

/**
 * @brief Central pipeline orchestrator.
 *
 *   Raw NMEA string → TelemetryParser → DataBuffer → AnomalyDetector
 *                                                          ↓
 *                                                   FrameCallback
 *
 * configure() must be called with the loaded sensor config before
 * start() — it initialises the generic TelemetryParser lookup tables.
 */
class SensorHub {
public:
    static constexpr std::size_t kBufferCapacity = 256;
    using FrameCallback = std::function<void(SensorFrame)>;

    explicit SensorHub();
    ~SensorHub();

    SensorHub(const SensorHub&)            = delete;
    SensorHub& operator=(const SensorHub&) = delete;

    /// Must be called before start() with the sensors loaded from JSON.
    void configure(const std::unordered_map<uint8_t, SensorConfig>& sensors);

    void start();
    void stop();

    void ingest(const std::string& raw);
    void setCallback(FrameCallback callback);

    AnomalyDetector& detector() noexcept { return m_detector; }

private:
    void processingLoop();

    TelemetryParser                          m_parser;
    AnomalyDetector                          m_detector;
    DataBuffer<SensorFrame, kBufferCapacity> m_buffer;
    FrameCallback                            m_callback;

    std::thread       m_worker;
    std::atomic<bool> m_running{false};
};

} // namespace nmea