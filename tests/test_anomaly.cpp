#include "nmea/AnomalyDetector.hpp"
#include <gtest/gtest.h>

using namespace nmea;

// Sensor IDs — match sensors.json
static constexpr uint8_t kEngineRpm   = 0x01;
static constexpr uint8_t kCoolantTemp = 0x02;
static constexpr uint8_t kWaterDepth  = 0x07;
static constexpr uint8_t kUnknown     = 0xFF;

class AnomalyDetectorTest : public ::testing::Test {
protected:
    AnomalyDetector detector;

    void SetUp() override {
        detector.setThreshold(kEngineRpm, {
            .warningLow=500, .warningHigh=1900,
            .criticalLow=200, .criticalHigh=2200});
        detector.setThreshold(kCoolantTemp, {
            .warningLow=60, .warningHigh=92,
            .criticalLow=40, .criticalHigh=105});
        detector.setThreshold(kWaterDepth, {
            .warningLow=5, .warningHigh=1000,
            .criticalLow=2, .criticalHigh=1000});
    }
};

TEST_F(AnomalyDetectorTest, NominalRpmIsNormal)
{
    SensorFrame f; f.sensorId = kEngineRpm; f.value = 1450.0;
    detector.evaluate(f);
    EXPECT_EQ(f.alertLevel, AlertLevel::Normal);
}

TEST_F(AnomalyDetectorTest, NominalCoolantTempIsNormal)
{
    SensorFrame f; f.sensorId = kCoolantTemp; f.value = 82.0;
    detector.evaluate(f);
    EXPECT_EQ(f.alertLevel, AlertLevel::Normal);
}

TEST_F(AnomalyDetectorTest, HighRpmTriggersWarning)
{
    SensorFrame f; f.sensorId = kEngineRpm; f.value = 1950.0;
    detector.evaluate(f);
    EXPECT_EQ(f.alertLevel, AlertLevel::Warning);
}

TEST_F(AnomalyDetectorTest, LowRpmTriggersWarning)
{
    SensorFrame f; f.sensorId = kEngineRpm; f.value = 450.0;
    detector.evaluate(f);
    EXPECT_EQ(f.alertLevel, AlertLevel::Warning);
}

TEST_F(AnomalyDetectorTest, OverheatWarning)
{
    SensorFrame f; f.sensorId = kCoolantTemp; f.value = 95.0;
    detector.evaluate(f);
    EXPECT_EQ(f.alertLevel, AlertLevel::Warning);
}

TEST_F(AnomalyDetectorTest, MaxRpmTriggersCritical)
{
    SensorFrame f; f.sensorId = kEngineRpm; f.value = 2300.0;
    detector.evaluate(f);
    EXPECT_EQ(f.alertLevel, AlertLevel::Critical);
}

TEST_F(AnomalyDetectorTest, CoolantOverheatCritical)
{
    SensorFrame f; f.sensorId = kCoolantTemp; f.value = 110.0;
    detector.evaluate(f);
    EXPECT_EQ(f.alertLevel, AlertLevel::Critical);
}

TEST_F(AnomalyDetectorTest, ShallowWaterCritical)
{
    SensorFrame f; f.sensorId = kWaterDepth; f.value = 1.5;
    detector.evaluate(f);
    EXPECT_EQ(f.alertLevel, AlertLevel::Critical);
}

TEST_F(AnomalyDetectorTest, CustomThresholdOverridesDefault)
{
    detector.setThreshold(kEngineRpm, {
        .warningLow=1000, .warningHigh=1600,
        .criticalLow=500, .criticalHigh=1800});
    SensorFrame f; f.sensorId = kEngineRpm; f.value = 1900.0;
    detector.evaluate(f);
    EXPECT_EQ(f.alertLevel, AlertLevel::Critical);
}

TEST_F(AnomalyDetectorTest, CustomStrategyIsUsed)
{
    detector.setStrategy([](const SensorFrame&, const SensorThreshold&) {
        return AlertLevel::Warning;
    });
    SensorFrame f; f.sensorId = kEngineRpm; f.value = 1450.0;
    detector.evaluate(f);
    EXPECT_EQ(f.alertLevel, AlertLevel::Warning);
}

TEST_F(AnomalyDetectorTest, UnknownSensorDefaultsToNormal)
{
    SensorFrame f; f.sensorId = kUnknown; f.value = 9999.0;
    detector.evaluate(f);
    EXPECT_EQ(f.alertLevel, AlertLevel::Normal);
}