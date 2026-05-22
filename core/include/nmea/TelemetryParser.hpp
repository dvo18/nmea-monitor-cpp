#pragma once

#include "nmea/SensorConfig.hpp"
#include "nmea/SensorFrame.hpp"

#include <optional>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

namespace nmea {

/**
 * @brief Generic NMEA-0183 parser driven entirely by sensors.json.
 *
 * No sensor-specific code exists here. For each incoming sentence the
 * parser:
 *   1. Validates the XOR checksum (NMEA-0183 v4.11 / IEC 61162-1).
 *   2. Extracts the sentence type identifier (e.g. "IIRPM", "IIMTW").
 *   3. Looks up which sensor is configured to listen to that sentence.
 *   4. Extracts the numeric value at the configured field index.
 *   5. Returns a SensorFrame with the sensor id and value.
 *
 * Adding a new instrument requires only a new entry in sensors.json —
 * no recompilation.
 *
 * For PSHIP sentences (proprietary multi-sensor) the first field is the
 * sensor id in hex, and nmeaField counts from after that prefix.
 */
class TelemetryParser {
public:
    /**
     * @brief Configure the parser from the loaded sensor config.
     *
     * Builds internal lookup tables:
     *   sentence type → list of (sensorId, fieldIndex) to extract
     */
    void configure(const std::unordered_map<uint8_t, SensorConfig>& sensors);

    /**
     * @brief Parse a single raw NMEA sentence.
     * @return A SensorFrame on success, std::nullopt on parse failure.
     */
    [[nodiscard]] std::optional<SensorFrame> parse(std::string_view raw) const;

    [[nodiscard]] static bool    validateChecksum(std::string_view sentence);
    [[nodiscard]] static uint8_t computeChecksum(std::string_view body);

private:
    struct SentenceMapping {
        uint8_t sensorId;
        int     fieldIndex;
        std::string unit;
    };

    // For PSHIP: key = sensorId hex string e.g. "03"
    // For others: key = sentence type e.g. "IIRPM"
    std::unordered_map<std::string, std::vector<SentenceMapping>> m_mappings;
    // PSHIP sensors: id hex → mapping
    std::unordered_map<std::string, SentenceMapping> m_pshipMappings;

    [[nodiscard]] std::optional<SensorFrame>
    parseSentence(std::string_view type, std::string_view fields) const;

    [[nodiscard]] std::optional<SensorFrame>
    parsePship(std::string_view fields) const;

    [[nodiscard]] static std::vector<std::string>
    split(std::string_view sv, char delim = ',');

    [[nodiscard]] static std::optional<double>
    extractField(const std::vector<std::string>& fields, int index);
};

} // namespace nmea