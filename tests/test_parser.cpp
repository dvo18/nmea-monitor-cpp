#include "nmea/TelemetryParser.hpp"
#include "nmea/SensorConfig.hpp"
#include "nmea/AnomalyDetector.hpp"
#include <gtest/gtest.h>
#include <cstdio>

using namespace nmea;

// Build a minimal config matching the test sentences
static LoadedConfig makeTestConfig()
{
    AnomalyDetector det;
    LoadedConfig cfg;

    auto add = [&](uint8_t id, const char* sentence, int field, const char* unit) {
        SensorConfig sc;
        sc.id           = id;
        sc.name         = "Sensor " + std::to_string(id);
        sc.nmeaSentence = sentence;
        sc.nmeaField    = field;
        sc.unit         = unit;
        cfg.sensors[id] = sc;
    };

    add(0x01, "IIRPM",  2, "RPM");
    add(0x02, "IIMTW",  0, "°C");
    add(0x07, "IIDPT",  0, "m");
    add(0x03, "PSHIP",  1, "bar");
    return cfg;
}

class TelemetryParserTest : public ::testing::Test {
protected:
    TelemetryParser parser;

    void SetUp() override {
        auto cfg = makeTestConfig();
        parser.configure(cfg.sensors);
    }
};

// ── Checksum ───────────────────────────────────────────────────────────────

TEST_F(TelemetryParserTest, ValidChecksumAccepted)
{
    const std::string body = "IIMTW,22.5,C";
    uint8_t cs = TelemetryParser::computeChecksum(body);
    char buf[64];
    std::snprintf(buf, sizeof(buf), "$%s*%02X", body.c_str(), cs);
    EXPECT_TRUE(TelemetryParser::validateChecksum(buf));
}

TEST_F(TelemetryParserTest, InvalidChecksumRejected)
{
    EXPECT_FALSE(TelemetryParser::validateChecksum("$IIMTW,22.5,C*00"));
}

TEST_F(TelemetryParserTest, MissingStarInvalidChecksum)
{
    EXPECT_FALSE(TelemetryParser::validateChecksum("$IIMTW,22.5,C"));
}

// ── IIRPM ──────────────────────────────────────────────────────────────────

TEST_F(TelemetryParserTest, ParsesRpmSentence)
{
    const std::string body = "IIRPM,E,1,1450.0,45.0,A";
    char buf[128];
    std::snprintf(buf, sizeof(buf), "$%s*%02X", body.c_str(),
                  TelemetryParser::computeChecksum(body));

    auto frame = parser.parse(buf);
    ASSERT_TRUE(frame.has_value());
    EXPECT_EQ(frame->sensorId, 0x01);
    EXPECT_DOUBLE_EQ(frame->value, 1450.0);
    EXPECT_EQ(frame->unit, "RPM");
}

TEST_F(TelemetryParserTest, RejectsRpmWithTooFewFields)
{
    const std::string body = "IIRPM,E,1";
    char buf[64];
    std::snprintf(buf, sizeof(buf), "$%s*%02X", body.c_str(),
                  TelemetryParser::computeChecksum(body));
    EXPECT_FALSE(parser.parse(buf).has_value());
}

// ── IIMTW ──────────────────────────────────────────────────────────────────

TEST_F(TelemetryParserTest, ParsesMtwSentence)
{
    const std::string body = "IIMTW,85.3,C";
    char buf[64];
    std::snprintf(buf, sizeof(buf), "$%s*%02X", body.c_str(),
                  TelemetryParser::computeChecksum(body));

    auto frame = parser.parse(buf);
    ASSERT_TRUE(frame.has_value());
    EXPECT_EQ(frame->sensorId, 0x02);
    EXPECT_NEAR(frame->value, 85.3, 1e-6);
}

// ── IIDPT ──────────────────────────────────────────────────────────────────

TEST_F(TelemetryParserTest, ParsesDptSentence)
{
    const std::string body = "IIDPT,18.5,0.0";
    char buf[64];
    std::snprintf(buf, sizeof(buf), "$%s*%02X", body.c_str(),
                  TelemetryParser::computeChecksum(body));

    auto frame = parser.parse(buf);
    ASSERT_TRUE(frame.has_value());
    EXPECT_EQ(frame->sensorId, 0x07);
    EXPECT_NEAR(frame->value, 18.5, 1e-6);
    EXPECT_EQ(frame->unit, "m");
}

// ── PSHIP ──────────────────────────────────────────────────────────────────

TEST_F(TelemetryParserTest, ParsesPshipFuelPressure)
{
    const std::string body = "PSHIP,03,5.50,bar";
    char buf[64];
    std::snprintf(buf, sizeof(buf), "$%s*%02X", body.c_str(),
                  TelemetryParser::computeChecksum(body));

    auto frame = parser.parse(buf);
    ASSERT_TRUE(frame.has_value());
    EXPECT_EQ(frame->sensorId, 0x03);
    EXPECT_NEAR(frame->value, 5.5, 1e-6);
}

TEST_F(TelemetryParserTest, RejectsUnknownSentence)
{
    const std::string body = "GPGGA,123519,4807.038";
    char buf[64];
    std::snprintf(buf, sizeof(buf), "$%s*%02X", body.c_str(),
                  TelemetryParser::computeChecksum(body));
    EXPECT_FALSE(parser.parse(buf).has_value());
}

TEST_F(TelemetryParserTest, RejectsEmptyInput)
{
    EXPECT_FALSE(parser.parse("").has_value());
    EXPECT_FALSE(parser.parse("   ").has_value());
}

TEST_F(TelemetryParserTest, RejectsMissingDollar)
{
    EXPECT_FALSE(parser.parse("IIMTW,22.5,C*2B").has_value());
}