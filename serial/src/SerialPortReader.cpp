#include "nmea/SerialPortReader.hpp"

#include <fcntl.h>
#include <termios.h>
#include <unistd.h>

#include <stdexcept>

namespace nmea {

SerialPortReader::SerialPortReader(std::string portPath, int baudRate)
    : m_portPath{std::move(portPath)}
    , m_baudRate{baudRate}
{}

SerialPortReader::~SerialPortReader()
{
    stop();
}

// ── Lifecycle ──────────────────────────────────────────────────────────────

void SerialPortReader::start(LineCallback callback)
{
    if (m_running.exchange(true)) return;

    m_fd = ::open(m_portPath.c_str(), O_RDONLY | O_NOCTTY | O_NONBLOCK);
    if (m_fd < 0)
        throw std::runtime_error{"SerialPortReader: cannot open " + m_portPath};

    if (!configurePort(m_fd)) {
        ::close(m_fd);
        m_fd = -1;
        throw std::runtime_error{"SerialPortReader: cannot configure " + m_portPath};
    }

    m_thread = std::thread{&SerialPortReader::readLoop, this, std::move(callback)};
}

void SerialPortReader::stop()
{
    m_running.store(false);
    if (m_fd >= 0) {
        ::close(m_fd);
        m_fd = -1;
    }
    if (m_thread.joinable()) m_thread.join();
}

// ── Read loop ──────────────────────────────────────────────────────────────

void SerialPortReader::readLoop(LineCallback callback)
{
    std::string line;
    line.reserve(128);
    char buf[256];

    while (m_running.load(std::memory_order_relaxed)) {
        const ssize_t n = ::read(m_fd, buf, sizeof(buf));

        if (n < 0) {
            // EAGAIN = no data yet on non-blocking fd — yield and retry
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                std::this_thread::sleep_for(std::chrono::milliseconds{10});
                continue;
            }
            break; // real error — exit loop
        }

        if (n == 0) {
            std::this_thread::sleep_for(std::chrono::milliseconds{10});
            continue;
        }

        // Accumulate characters into line; dispatch on newline
        for (ssize_t i = 0; i < n; ++i) {
            const char c = buf[i];
            if (c == '\n') {
                // Strip trailing \r if present
                if (!line.empty() && line.back() == '\r')
                    line.pop_back();
                if (!line.empty() && callback)
                    callback(line);
                line.clear();
            } else {
                line += c;
            }
        }
    }
}

// ── Port configuration ─────────────────────────────────────────────────────

bool SerialPortReader::configurePort(int fd) const
{
    struct termios tty{};
    if (::tcgetattr(fd, &tty) != 0) return false;

    const int speed = toBaudConstant(m_baudRate);
    ::cfsetispeed(&tty, speed);
    ::cfsetospeed(&tty, speed);

    // Raw mode — no echo, no signals, no processing
    ::cfmakeraw(&tty);

    // 8N1
    tty.c_cflag &= ~static_cast<tcflag_t>(CSIZE);
    tty.c_cflag |= CS8;
    tty.c_cflag &= ~static_cast<tcflag_t>(PARENB);
    tty.c_cflag &= ~static_cast<tcflag_t>(CSTOPB);
    tty.c_cflag |= CLOCAL | CREAD;

    // Non-blocking reads
    tty.c_cc[VMIN]  = 0;
    tty.c_cc[VTIME] = 0;

    return ::tcsetattr(fd, TCSANOW, &tty) == 0;
}

int SerialPortReader::toBaudConstant(int baudRate)
{
    switch (baudRate) {
        case 4800:   return B4800;
        case 9600:   return B9600;
        case 19200:  return B19200;
        case 38400:  return B38400;
        case 115200: return B115200;
        default:     return B4800;
    }
}

} // namespace nmea