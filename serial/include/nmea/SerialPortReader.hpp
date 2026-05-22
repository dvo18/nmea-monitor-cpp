#pragma once

#include <atomic>
#include <functional>
#include <string>
#include <thread>

namespace nmea {

/**
 * @brief Reads NMEA-0183 sentences from a serial port (real or virtual).
 *
 * Opens the given port path, reads line by line, and invokes a callback
 * for each complete sentence. The read loop runs in a dedicated thread.
 *
 * This class is the single point of change between simulation and
 * production:
 *
 *   // Simulation (development)
 *   SerialPortReader reader{writer.slavePath()};
 *
 *   // Production (real instrument on USB-serial adapter)
 *   SerialPortReader reader{"/dev/ttyUSB0"};
 *
 * The callback signature and the rest of the pipeline are identical in
 * both cases.
 *
 * Default baud rate: 4800 bps — the NMEA-0183 standard rate.
 * High-speed NMEA (38400 bps) is supported via the constructor parameter.
 */
class SerialPortReader {
public:
    using LineCallback = std::function<void(const std::string& sentence)>;

    explicit SerialPortReader(std::string portPath, int baudRate = 4800);
    ~SerialPortReader();

    // Non-copyable
    SerialPortReader(const SerialPortReader&)            = delete;
    SerialPortReader& operator=(const SerialPortReader&) = delete;

    // ── Lifecycle ──────────────────────────────────────────────────────────

    /// Start reading. Callback is invoked from the reader thread.
    void start(LineCallback callback);

    /// Stop the reader thread and close the port.
    void stop();

    [[nodiscard]] bool isRunning() const noexcept { return m_running.load(); }
    [[nodiscard]] const std::string& portPath() const noexcept { return m_portPath; }

private:
    void readLoop(LineCallback callback);

    /// Configure the file descriptor as a raw serial port at m_baudRate.
    bool configurePort(int fd) const;

    /// Convert integer baud rate to POSIX speed_t constant.
    static int toBaudConstant(int baudRate);

    std::string       m_portPath;
    int               m_baudRate;
    int               m_fd{-1};
    std::thread       m_thread;
    std::atomic<bool> m_running{false};
};

} // namespace nmea