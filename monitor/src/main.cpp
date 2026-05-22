#include "SensorModel.hpp"

#include "nmea/AlertTracker.hpp"
#include "nmea/SensorConfig.hpp"
#include "nmea/SensorHub.hpp"
#include "nmea/SerialPortReader.hpp"
#include "nmea/SessionLogger.hpp"

#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <cstdlib>
#include <iostream>

static std::string thresholdDesc(const nmea::AlertTracker::AlertEvent& ev,
                                  const nmea::SensorConfig& cfg)
{
    const auto& th = cfg.threshold;
    if (ev.openValue >= th.criticalHigh)
        return ">= " + std::to_string(th.criticalHigh) + " (critical high)";
    if (ev.openValue <= th.criticalLow)
        return "<= " + std::to_string(th.criticalLow)  + " (critical low)";
    if (ev.openValue >= th.warningHigh)
        return ">= " + std::to_string(th.warningHigh)  + " (warning high)";
    return "<= " + std::to_string(th.warningLow) + " (warning low)";
}

int main(int argc, char* argv[])
{
    QGuiApplication app{argc, argv};
    app.setApplicationName("NMEA Monitor");
    app.setApplicationVersion("1.0.0");
    app.setOrganizationName("nmea-monitor-cpp");

    // ── Load configuration ─────────────────────────────────────────────────
    const char* envCfg = std::getenv("NMEA_CONFIG_DIR");
    const std::string cfgPath =
        std::string(envCfg && envCfg[0] != '\0' ? envCfg : ".") + "/sensors.json";

    nmea::SensorHub hub;
    nmea::LoadedConfig cfg = nmea::SensorConfigLoader::load(cfgPath,
                                                             hub.detector());
    // Configure the generic parser from sensors.json
    hub.configure(cfg.sensors);

    const std::string port     = cfg.connection.port;
    const int         baudRate = cfg.connection.baudRate;

    std::cout << "[nmea-monitor] listening on " << port
              << " @ " << baudRate << " baud\n";

    // ── Serial reader ──────────────────────────────────────────────────────
    nmea::SerialPortReader reader{port, baudRate};

    // ── Pipeline ───────────────────────────────────────────────────────────
    nmea::SensorModel   sensorModel;
    nmea::EventLogModel eventLog;
    nmea::AlertTracker  tracker;

    const char* envLog = std::getenv("NMEA_LOG_DIR");
    const std::string logDir = (envLog && envLog[0] != '\0') ? envLog : "logs";
    nmea::SessionLogger logger{logDir};

    sensorModel.setConfigs(cfg.sensors);

    tracker.setOnAlertOpened([&](const nmea::AlertTracker::AlertEvent& ev) {
        const uint8_t id = ev.sensorId;
        std::string desc;
        if (auto it = cfg.sensors.find(id); it != cfg.sensors.end())
            desc = thresholdDesc(ev, it->second);
        logger.logOpen(ev, desc);
        QMetaObject::invokeMethod(&eventLog,
            [&eventLog, ev, desc] { eventLog.onAlertOpened(ev, desc); },
            Qt::QueuedConnection);
    });

    tracker.setOnAlertClosed([&](const nmea::AlertTracker::AlertEvent& ev) {
        logger.logClose(ev);
        QMetaObject::invokeMethod(&eventLog,
            [&eventLog, ev] { eventLog.onAlertClosed(ev); },
            Qt::QueuedConnection);
    });

    hub.setCallback([&](nmea::SensorFrame frame) {
        // Look up sensor name from config
        std::string sensorName;
        if (auto it = cfg.sensors.find(frame.sensorId); it != cfg.sensors.end())
            sensorName = it->second.name;

        tracker.process(frame, sensorName);
        sensorModel.pushFrame(std::move(frame));
    });

    reader.start([&hub](const std::string& sentence) {
        hub.ingest(sentence);
    });

    hub.start();

    // ── Qt / QML ───────────────────────────────────────────────────────────
    QQmlApplicationEngine engine;
    engine.rootContext()->setContextProperty("sensorModel",   &sensorModel);
    engine.rootContext()->setContextProperty("eventLogModel", &eventLog);
    engine.rootContext()->setContextProperty("serialPort",
        QString::fromStdString(port));

    const QUrl url{QStringLiteral("qrc:/qml/Main.qml")};
    QObject::connect(&engine, &QQmlApplicationEngine::objectCreated,
        &app, [url](QObject* obj, const QUrl& objUrl) {
            if (!obj && url == objUrl) QCoreApplication::exit(-1);
        }, Qt::QueuedConnection);

    engine.load(url);

    const int ret = app.exec();

    logger.flushOpenEvents(tracker.openEvents());
    hub.stop();
    reader.stop();
    return ret;
}