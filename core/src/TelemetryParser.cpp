#include "nmea/TelemetryParser.hpp"

#include <charconv>
#include <cstdint>
#include <iostream>
#include <sstream>

namespace nmea {

// ── Configuration ──────────────────────────────────────────────────────────

void TelemetryParser::configure(
    const std::unordered_map<uint8_t, SensorConfig>& sensors)
{
    m_mappings.clear();
    m_pshipMappings.clear();

    for (const auto& [id, cfg] : sensors) {
        if (cfg.nmeaSentence == "PSHIP") {
            char hexId[3];
            std::snprintf(hexId, sizeof(hexId), "%02X", id);
            m_pshipMappings[hexId] = {id, cfg.nmeaField, cfg.unit};
        } else {
            m_mappings[cfg.nmeaSentence].push_back(
                {id, cfg.nmeaField, cfg.unit});
        }
    }
}

// ── Checksum ───────────────────────────────────────────────────────────────

uint8_t TelemetryParser::computeChecksum(std::string_view body)
{
    uint8_t cs = 0;
    for (unsigned char c : body) cs ^= c;
    return cs;
}

bool TelemetryParser::validateChecksum(std::string_view sentence)
{
    const auto dollarPos = sentence.find('$');
    const auto starPos   = sentence.rfind('*');
    if (dollarPos == std::string_view::npos ||
        starPos   == std::string_view::npos ||
        starPos + 2 >= sentence.size()) return false;

    const auto body     = sentence.substr(dollarPos + 1,
                                          starPos - dollarPos - 1);
    const auto hexChars = sentence.substr(starPos + 1, 2);

    uint8_t expected = 0;
    const auto [ptr, ec] = std::from_chars(
        hexChars.data(), hexChars.data() + 2, expected, 16);
    if (ec != std::errc{}) return false;

    return computeChecksum(body) == expected;
}

// ── Helpers ────────────────────────────────────────────────────────────────

std::vector<std::string> TelemetryParser::split(std::string_view sv, char delim)
{
    std::vector<std::string> tokens;
    std::size_t start = 0;
    while (start <= sv.size()) {
        auto end = sv.find(delim, start);
        if (end == std::string_view::npos) end = sv.size();
        tokens.emplace_back(sv.substr(start, end - start));
        start = end + 1;
    }
    return tokens;
}

std::optional<double> TelemetryParser::extractField(
    const std::vector<std::string>& fields, int index)
{
    if (index < 0 || index >= static_cast<int>(fields.size()))
        return std::nullopt;

    const auto& f = fields[static_cast<std::size_t>(index)];
    try {
        std::size_t pos = 0;
        return std::stod(f, &pos);
    } catch (...) {
        return std::nullopt;
    }
}

// ── Parse entry point ──────────────────────────────────────────────────────

std::optional<SensorFrame> TelemetryParser::parse(std::string_view raw) const
{
    while (!raw.empty() &&
           (raw.back() == '\r' || raw.back() == '\n' || raw.back() == ' '))
        raw.remove_suffix(1);

    if (raw.size() < 2 || raw.front() != '$') return std::nullopt;

    if (raw.find('*') != std::string_view::npos && !validateChecksum(raw))
        return std::nullopt;

    auto stripped = raw.substr(1);
    if (const auto star = stripped.rfind('*');
        star != std::string_view::npos)
        stripped = stripped.substr(0, star);

    const auto comma = stripped.find(',');
    if (comma == std::string_view::npos) return std::nullopt;

    const std::string type{stripped.substr(0, comma)};
    const auto        rest = stripped.substr(comma + 1);

    if (type == "PSHIP") return parsePship(rest);
    return parseSentence(type, rest);
}

// ── Generic sentence parser ────────────────────────────────────────────────

std::optional<SensorFrame> TelemetryParser::parseSentence(
    std::string_view type, std::string_view fields) const
{
    const auto it = m_mappings.find(std::string{type});
    if (it == m_mappings.end()) return std::nullopt;

    const auto tokens = split(fields);

    for (const auto& mapping : it->second) {
        const auto value = extractField(tokens, mapping.fieldIndex);
        if (!value) continue;

        SensorFrame frame;
        frame.sensorId = mapping.sensorId;
        frame.value    = *value;
        frame.unit     = mapping.unit;
        return frame;
    }
    return std::nullopt;
}

// ── PSHIP proprietary sentence ─────────────────────────────────────────────

std::optional<SensorFrame> TelemetryParser::parsePship(
    std::string_view fields) const
{
    const auto tokens = split(fields);
    if (tokens.size() < 2) return std::nullopt;

    const std::string& hexId = tokens[0];
    const auto it = m_pshipMappings.find(hexId);
    if (it == m_pshipMappings.end()) return std::nullopt;

    const auto& mapping = it->second;
    const auto value = extractField(tokens, mapping.fieldIndex);
    if (!value) return std::nullopt;

    SensorFrame frame;
    frame.sensorId = mapping.sensorId;
    frame.value    = *value;
    frame.unit     = mapping.unit;
    return frame;
}

} // namespace nmea