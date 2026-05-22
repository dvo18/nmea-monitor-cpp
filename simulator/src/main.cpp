#include "nmea/SensorSimulator.hpp"

#include <atomic>
#include <chrono>
#include <csignal>
#include <cstring>
#include <fcntl.h>
#include <iostream>
#include <stdexcept>
#include <string>
#include <thread>
#include <unistd.h>

#if defined(__linux__)
#  include <pty.h>
#else
#  include <util.h>
#endif

namespace {
    std::atomic<bool> g_running{true};
}

static void onSignal(int) { g_running.store(false); }

/**
 * @brief Opens a POSIX pseudoterminal master/slave pair.
 *
 * The simulator writes to the master fd.
 * The slave path (e.g. /dev/pts/3) is printed to stdout so the
 * Makefile can read it and update sensors.json before launching
 * the monitor.
 *
 * From the monitor's perspective the slave path is indistinguishable
 * from a real serial port like /dev/ttyUSB0.
 */
static std::pair<int, std::string> openPty()
{
    const int master = ::posix_openpt(O_RDWR | O_NOCTTY);
    if (master < 0)
        throw std::runtime_error{std::string{"posix_openpt: "} + strerror(errno)};

    if (::grantpt(master) != 0 || ::unlockpt(master) != 0) {
        ::close(master);
        throw std::runtime_error{"grantpt/unlockpt failed"};
    }

    const char* slave = ::ptsname(master);
    if (!slave) {
        ::close(master);
        throw std::runtime_error{"ptsname failed"};
    }

    return {master, std::string{slave}};
}

int main()
{
    std::signal(SIGINT,  onSignal);
    std::signal(SIGTERM, onSignal);

    try {
        auto [masterFd, slavePath] = openPty();

        // Print slave path to stdout — Makefile reads this to configure
        // the monitor's port before launching it.
        // Format: "pty:<path>" for easy parsing.
        std::cout << "pty:" << slavePath << std::endl;

        std::cerr << "[nmea-simulator] pty slave: " << slavePath << "\n";
        std::cerr << "[nmea-simulator] writing NMEA sentences...\n";

        // Verify the pty master is writable before starting.
        // The slave end must be opened by the monitor first.
        // Retry for up to 5 seconds.
        {
            bool ready = false;
            for (int attempt = 0; attempt < 50; ++attempt) {
                // Try a zero-byte write — succeeds if slave is open
                const ssize_t n = ::write(masterFd, "", 0);
                if (n >= 0) { ready = true; break; }
                if (errno != EIO) { ready = true; break; } // EIO = no reader yet
                std::this_thread::sleep_for(std::chrono::milliseconds{100});
            }
            if (!ready) {
                std::cerr << "[nmea-simulator] warning: no reader detected on "
                          << slavePath << " — starting anyway\n";
            } else {
                std::cerr << "[nmea-simulator] slave port is open, starting emission\n";
            }
        }

        nmea::SensorSimulator simulator{masterFd};
        simulator.start();

        while (g_running.load())
            std::this_thread::sleep_for(std::chrono::milliseconds{100});

        simulator.stop();
        ::close(masterFd);
        std::cerr << "[nmea-simulator] stopped\n";
    }
    catch (const std::exception& e) {
        std::cerr << "[nmea-simulator] error: " << e.what() << "\n";
        return 1;
    }

    return 0;
}