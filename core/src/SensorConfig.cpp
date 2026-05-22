#include "nmea/SensorConfig.hpp"

#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <iostream>

namespace nmea {

LoadedConfig SensorConfigLoader::load(const std::string& path,
                                       AnomalyDetector&   detector)
{
    LoadedConfig result;

    QFile file{QString::fromStdString(path)};
    if (!file.open(QIODevice::ReadOnly)) {
        std::cerr << "[SensorConfig] cannot open " << path
                  << " — using defaults\n";
        return result;
    }

    const QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
    if (doc.isNull() || !doc.isObject()) {
        std::cerr << "[SensorConfig] invalid JSON in " << path << "\n";
        return result;
    }

    const QJsonObject root = doc.object();

    // ── Connection ─────────────────────────────────────────────────────────
    if (root.contains("connection")) {
        const QJsonObject conn = root.value("connection").toObject();
        result.connection.port     = conn.value("port").toString("/dev/ttyUSB0").toStdString();
        result.connection.baudRate = conn.value("baudRate").toInt(4800);
        result.connection.notes    = conn.value("notes").toString().toStdString();
    }

    // ── Sensors ────────────────────────────────────────────────────────────
    const QJsonArray sensors = root.value("sensors").toArray();
    for (const QJsonValue& val : sensors) {
        const QJsonObject obj = val.toObject();

        bool ok = false;
        const uint8_t id = static_cast<uint8_t>(
            obj.value("id").toString().toUInt(&ok, 16));
        if (!ok) continue;

        const QJsonObject th = obj.value("thresholds").toObject();

        SensorConfig cfg;
        cfg.id           = id;
        cfg.name         = obj.value("name").toString().toStdString();
        cfg.nmeaSentence = obj.value("nmeaSentence").toString().toStdString();
        cfg.nmeaField    = obj.value("nmeaField").toInt(0);
        cfg.unit         = obj.value("unit").toString().toStdString();
        cfg.updateHz     = obj.value("updateHz").toInt(1);
        cfg.notes        = obj.value("notes").toString().toStdString();

        cfg.threshold.warningLow   = th.value("warningLow").toDouble(-1e9);
        cfg.threshold.warningHigh  = th.value("warningHigh").toDouble(1e9);
        cfg.threshold.criticalLow  = th.value("criticalLow").toDouble(-1e9);
        cfg.threshold.criticalHigh = th.value("criticalHigh").toDouble(1e9);

        detector.setThreshold(id, cfg.threshold);
        result.sensors[id] = std::move(cfg);
    }

    std::cout << "[SensorConfig] loaded " << result.sensors.size()
              << " sensors from " << path
              << " — port: " << result.connection.port << "\n";

    return result;
}

} // namespace nmea