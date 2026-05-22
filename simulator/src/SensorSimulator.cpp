#include "nmea/SensorSimulator.hpp"

#include <cmath>
#include <cstdio>
#include <iomanip>
#include <sstream>
#include <unistd.h>

namespace nmea {

static constexpr int kBaseTickMs     = 100;
static constexpr int kTickRpm        = 1;
static constexpr int kTickVibration  = 2;
static constexpr int kTickTorque     = 2;
static constexpr int kTickWind       = 5;
static constexpr int kTickFuel       = 5;
static constexpr int kTickTemp       = 10;
static constexpr int kTickDepth      = 10;
static constexpr int kInterSentenceMs = 12;

SensorSimulator::SensorSimulator(int masterFd)
    : m_fd{masterFd}
    , m_rng{std::random_device{}()}
{}

SensorSimulator::~SensorSimulator() { stop(); }

void SensorSimulator::start()
{
    if (m_running.exchange(true)) return;
    m_worker = std::thread{&SensorSimulator::emitLoop, this};
}

void SensorSimulator::stop()
{
    m_running.store(false);
    if (m_worker.joinable()) m_worker.join();
}

// ── Write ──────────────────────────────────────────────────────────────────

void SensorSimulator::writeSentence(const std::string& sentence)
{
    const std::string line = sentence + "\r\n";
    const char* ptr = line.data();
    std::size_t remaining = line.size();

    while (remaining > 0) {
        const ssize_t n = ::write(m_fd, ptr, remaining);
        if (n < 0) {
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                std::this_thread::sleep_for(std::chrono::milliseconds{1});
                continue;
            }
            return;
        }
        ptr       += n;
        remaining -= static_cast<std::size_t>(n);
    }
    std::this_thread::sleep_for(std::chrono::milliseconds{kInterSentenceMs});
}

// ── Emit loop ──────────────────────────────────────────────────────────────

void SensorSimulator::emitLoop()
{
    while (m_running.load(std::memory_order_relaxed)) {
        ++m_tick;

        if (m_tick % kTickRpm       == 0)
            writeSentence(buildRpmSentence(modelEngineRpm()));
        if (m_tick % kTickTemp      == 0)
            writeSentence(buildMtwSentence(modelCoolantTemp()));
        if (m_tick % kTickDepth     == 0)
            writeSentence(buildDptSentence(modelWaterDepth()));
        if (m_tick % kTickWind      == 0) {
            writeSentence(buildPshipSentence(
                0x05,
                modelWindSpeed(), "m/s"));
            writeSentence(buildPshipSentence(
                0x06,
                modelWindDirection(), "deg"));
        }
        if (m_tick % kTickFuel      == 0)
            writeSentence(buildPshipSentence(
                0x03,
                modelFuelPressure(), "bar"));
        if (m_tick % kTickVibration == 0)
            writeSentence(buildPshipSentence(
                0x04,
                modelHullVibration(), "mm/s"));
        if (m_tick % kTickTorque    == 0)
            writeSentence(buildPshipSentence(
                0x08,
                modelPropellerTorque(), "%"));

        std::this_thread::sleep_for(std::chrono::milliseconds{kBaseTickMs});
    }
}

// ── Physical models ────────────────────────────────────────────────────────

double SensorSimulator::modelEngineRpm()
{
    std::normal_distribution<double> noise{0.0, 8.0};
    if (m_rpmHoldTicks <= 0) {
        const double r = std::uniform_real_distribution<double>{0.0, 1.0}(m_rng);
        if      (r < 0.65) m_rpmTarget = std::uniform_real_distribution<double>{900.0,  1800.0}(m_rng);
        else if (r < 0.85) m_rpmTarget = std::uniform_real_distribution<double>{1800.0, 2100.0}(m_rng);
        else               m_rpmTarget = std::uniform_real_distribution<double>{2100.0, 2500.0}(m_rng);
        m_rpmHoldTicks = std::uniform_int_distribution<int>{150, 400}(m_rng);
    }
    --m_rpmHoldTicks;
    m_rpm += (m_rpmTarget - m_rpm) * 0.04 + noise(m_rng);
    m_rpm  = std::clamp(m_rpm, 150.0, 2600.0);
    return m_rpm;
}

double SensorSimulator::modelCoolantTemp()
{
    std::normal_distribution<double> noise{0.0, 0.1};
    const double load = std::clamp((m_rpm - 600.0) / 1400.0, 0.0, 1.0);
    m_coolantTemp += (72.0 + load * 36.0 - m_coolantTemp) * 0.005 + noise(m_rng);
    return m_coolantTemp;
}

double SensorSimulator::modelWaterDepth()
{
    std::normal_distribution<double> noise{0.0, 0.08};
    m_depthPhase += 0.0087;
    m_waterDepth  = std::max(18.0 + 15.0 * std::sin(m_depthPhase) + noise(m_rng), 0.3);
    return m_waterDepth;
}

double SensorSimulator::modelWindSpeed()
{
    std::normal_distribution<double> noise{0.0, 0.2};
    if (m_gustTicks <= 0) {
        if (std::uniform_real_distribution<double>{0.0, 1.0}(m_rng) < 0.025) {
            const double r = std::uniform_real_distribution<double>{0.0, 1.0}(m_rng);
            m_gustTarget = (r < 0.65)
                ? std::uniform_real_distribution<double>{10.0, 14.0}(m_rng)
                : std::uniform_real_distribution<double>{14.0, 21.0}(m_rng);
            m_gustTicks = std::uniform_int_distribution<int>{20, 50}(m_rng);
        } else {
            m_gustTarget = std::uniform_real_distribution<double>{4.0, 10.0}(m_rng);
        }
    }
    --m_gustTicks;
    m_windSpeed += (m_gustTarget - m_windSpeed) * 0.15 + noise(m_rng);
    m_windSpeed  = std::clamp(m_windSpeed, 0.0, 25.0);
    return m_windSpeed;
}

double SensorSimulator::modelWindDirection()
{
    std::normal_distribution<double> noise{0.0, 1.2};
    m_windDir += noise(m_rng);
    if (m_windDir <   0.0) m_windDir += 360.0;
    if (m_windDir > 360.0) m_windDir -= 360.0;
    return m_windDir;
}

double SensorSimulator::modelFuelPressure()
{
    std::normal_distribution<double> noise{0.0, 0.02};
    const double load = std::clamp((m_rpm - 600.0) / 1400.0, 0.0, 1.0);
    m_fuelPressure += (4.5 + load * 2.5 - m_fuelPressure) * 0.03 + noise(m_rng);
    if (std::uniform_real_distribution<double>{0.0, 1.0}(m_rng) < 0.015)
        m_fuelPressure -= std::uniform_real_distribution<double>{0.5, 2.0}(m_rng);
    m_fuelPressure = std::clamp(m_fuelPressure, 0.5, 11.0);
    return m_fuelPressure;
}

double SensorSimulator::modelHullVibration()
{
    std::normal_distribution<double> noise{0.0, 0.06};
    const double load = std::clamp((m_rpm - 600.0) / 1400.0, 0.0, 1.0);
    m_vibration += (0.4 + load * 9.5 - m_vibration) * 0.12 + noise(m_rng);
    m_vibration  = std::clamp(m_vibration, 0.0, 13.0);
    return m_vibration;
}

double SensorSimulator::modelPropellerTorque()
{
    std::normal_distribution<double> noise{0.0, 0.6};
    const double load = std::clamp((m_rpm - 600.0) / 1400.0, 0.0, 1.0);
    m_torque += (15.0 + load * 82.0 - m_torque) * 0.10 + noise(m_rng);
    m_torque  = std::clamp(m_torque, 0.0, 105.0);
    return m_torque;
}

// ── NMEA builders ──────────────────────────────────────────────────────────

uint8_t SensorSimulator::computeChecksum(const std::string& body)
{
    uint8_t cs = 0;
    for (unsigned char c : body) cs ^= c;
    return cs;
}

std::string SensorSimulator::toHex(uint8_t v)
{
    char buf[3];
    std::snprintf(buf, sizeof(buf), "%02X", v);
    return {buf};
}

std::string SensorSimulator::buildRpmSentence(double rpm) const
{
    std::ostringstream body;
    body << "IIRPM,E,1," << std::fixed << std::setprecision(1) << rpm << ",45.0,A";
    const std::string b = body.str();
    return "$" + b + "*" + toHex(computeChecksum(b));
}

std::string SensorSimulator::buildMtwSentence(double tempC) const
{
    std::ostringstream body;
    body << "IIMTW," << std::fixed << std::setprecision(1) << tempC << ",C";
    const std::string b = body.str();
    return "$" + b + "*" + toHex(computeChecksum(b));
}

std::string SensorSimulator::buildDptSentence(double depthM) const
{
    std::ostringstream body;
    body << "IIDPT," << std::fixed << std::setprecision(1) << depthM << ",0.0";
    const std::string b = body.str();
    return "$" + b + "*" + toHex(computeChecksum(b));
}

std::string SensorSimulator::buildPshipSentence(uint8_t id, double value,
                                                  const std::string& unit) const
{
    std::ostringstream body;
    body << "PSHIP," << toHex(id) << ","
         << std::fixed << std::setprecision(2) << value << "," << unit;
    const std::string b = body.str();
    return "$" + b + "*" + toHex(computeChecksum(b));
}

} // namespace nmea