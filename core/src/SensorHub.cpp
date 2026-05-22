#include "nmea/SensorHub.hpp"

namespace nmea {

SensorHub::SensorHub() = default;

SensorHub::~SensorHub() { stop(); }

void SensorHub::configure(
    const std::unordered_map<uint8_t, SensorConfig>& sensors)
{
    m_parser.configure(sensors);
}

void SensorHub::start()
{
    if (m_running.exchange(true)) return;
    m_worker = std::thread{&SensorHub::processingLoop, this};
}

void SensorHub::stop()
{
    if (!m_running.exchange(false)) return;

    // Push sentinel to unblock the worker
    SensorFrame sentinel;
    sentinel.sensorId = 0xFF;
    m_buffer.tryPush(std::move(sentinel));

    if (m_worker.joinable()) m_worker.join();
}

void SensorHub::ingest(const std::string& raw)
{
    auto frame = m_parser.parse(raw);
    if (!frame) return;
    m_buffer.tryPush(std::move(*frame));
}

void SensorHub::setCallback(FrameCallback callback)
{
    m_callback = std::move(callback);
}

void SensorHub::processingLoop()
{
    while (m_running.load(std::memory_order_relaxed)) {
        SensorFrame frame = m_buffer.pop();
        if (frame.sensorId == 0xFF) break; // sentinel

        m_detector.evaluate(frame);
        if (m_callback) m_callback(std::move(frame));
    }
}

} // namespace nmea