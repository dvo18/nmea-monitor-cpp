#pragma once

#include <atomic>
#include <chrono>
#include <random>
#include <string>
#include <thread>

namespace nmea {

/**
 * @brief Simulates a multi-sensor instrument bus by writing NMEA-0183
 *        sentences to a file descriptor (serial port master fd).
 *
 * The fd is provided externally by main.cpp, which opens the pty master
 * using posix_openpt(). This keeps the simulator decoupled from port
 * management — it only knows how to generate and write NMEA sentences.
 *
 * Each channel emits at its own realistic rate (base tick 100ms):
 *   Engine RPM       10 Hz
 *   Hull Vibration    5 Hz
 *   Propeller Torque  5 Hz
 *   Wind Speed        2 Hz
 *   Wind Direction    2 Hz
 *   Fuel Pressure     2 Hz
 *   Coolant Temp      1 Hz
 *   Water Depth       1 Hz
 */
class SensorSimulator {
public:
    /// @param masterFd  File descriptor to write NMEA sentences to.
    ///                  Typically the master end of a POSIX pty pair.
    explicit SensorSimulator(int masterFd);
    ~SensorSimulator();

    SensorSimulator(const SensorSimulator&)            = delete;
    SensorSimulator& operator=(const SensorSimulator&) = delete;

    void start();
    void stop();

    [[nodiscard]] bool isRunning() const noexcept { return m_running.load(); }

private:
    void emitLoop();
    void writeSentence(const std::string& sentence);

    double modelEngineRpm();
    double modelCoolantTemp();
    double modelWaterDepth();
    double modelWindSpeed();
    double modelWindDirection();
    double modelFuelPressure();
    double modelHullVibration();
    double modelPropellerTorque();

    std::string buildRpmSentence(double rpm)    const;
    std::string buildMtwSentence(double tempC)  const;
    std::string buildDptSentence(double depthM) const;
    std::string buildPshipSentence(uint8_t id,
                                   double value,
                                   const std::string& unit) const;

    static std::string toHex(uint8_t v);
    static uint8_t     computeChecksum(const std::string& body);

    int               m_fd;
    std::mt19937      m_rng;
    std::thread       m_worker;
    std::atomic<bool> m_running{false};

    // Physical state
    double m_rpm          {1450.0};
    double m_coolantTemp  {  82.0};
    double m_waterDepth   {  18.5};
    double m_windSpeed    {   8.0};
    double m_windDir      { 270.0};
    double m_fuelPressure {   5.5};
    double m_vibration    {   1.2};
    double m_torque       {  72.0};

    double m_rpmTarget    {1450.0};
    int    m_rpmHoldTicks {0};
    double m_depthPhase   {0.0};
    int    m_gustTicks    {0};
    double m_gustTarget   {8.0};
    int    m_tick         {0};
};

} // namespace nmea