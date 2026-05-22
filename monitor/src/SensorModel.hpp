#pragma once

#include "nmea/SensorFrame.hpp"
#include "nmea/SensorConfig.hpp"
#include "nmea/AlertTracker.hpp"

#include <QAbstractListModel>
#include <QDateTime>
#include <QObject>
#include <QTimer>
#include <deque>
#include <unordered_map>

namespace nmea {

// ── SensorModel ────────────────────────────────────────────────────────────

class SensorModel : public QAbstractListModel {
    Q_OBJECT
    Q_PROPERTY(int count         READ rowCount      NOTIFY countChanged)
    Q_PROPERTY(int criticalCount READ criticalCount NOTIFY alertCountChanged)
    Q_PROPERTY(int warningCount  READ warningCount  NOTIFY alertCountChanged)

public:
    enum Roles {
        SensorNameRole = Qt::UserRole + 1,
        ValueRole,
        UnitRole,
        AlertLevelRole,
        TimestampRole,
        WarnLowRole,
        WarnHighRole,
        CritLowRole,
        CritHighRole,
        HasDataRole,
    };

    explicit SensorModel(QObject* parent = nullptr);

    void setConfigs(const std::unordered_map<uint8_t, SensorConfig>& configs);

    int      rowCount(const QModelIndex& parent = {}) const override;
    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
    QHash<int, QByteArray> roleNames() const override;

    void pushFrame(SensorFrame frame);

    int criticalCount() const { return m_criticalCount; }
    int warningCount()  const { return m_warningCount;  }

signals:
    void countChanged();
    void alertCountChanged();
    void criticalAlert(const QString& sensorName, double value, const QString& unit);

private:
    void pushFrameOnMainThread(SensorFrame frame);
    void recomputeAlertCounts();

    struct Row {
        QString sensorName;
        double  value      {0.0};
        QString unit;
        int     alertLevel {0};
        QString timestamp;
        double  warnLow    {-1e9};
        double  warnHigh   { 1e9};
        double  critLow    {-1e9};
        double  critHigh   { 1e9};
        bool    hasData    {false}; ///< false until first frame received
    };

    std::deque<Row>                           m_rows;
    std::unordered_map<uint8_t, SensorConfig> m_configs;
    int                                       m_criticalCount{0};
    int                                       m_warningCount {0};
};

// ── EventLogModel ──────────────────────────────────────────────────────────

/**
 * @brief Live event log shown in the UI.
 *
 * Shows at most kMaxEvents entries, newest first (FIFO queue).
 * Each entry has a state: ACTIVE (alert open) or CLOSED (resolved).
 * ACTIVE entries show a live duration counter updated every second.
 * When an alert closes, its entry transitions to CLOSED with final duration.
 */
class EventLogModel : public QAbstractListModel {
    Q_OBJECT
    Q_PROPERTY(int count READ rowCount NOTIFY countChanged)

public:
    enum Roles {
        EventTimeRole = Qt::UserRole + 1,
        EventSensorRole,
        EventValueRole,
        EventUnitRole,
        EventLevelRole,
        EventThreshRole,
        EventStatusRole,    // "ACTIVE" or "CLOSED"
        EventDurationRole,  // "HH:MM:SS"
    };

    static constexpr int kMaxEvents = 50;

    explicit EventLogModel(QObject* parent = nullptr);

    int      rowCount(const QModelIndex& parent = {}) const override;
    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
    QHash<int, QByteArray> roleNames() const override;

    /// Called when an alert opens — adds ACTIVE entry to the top.
    void onAlertOpened(const AlertTracker::AlertEvent& event,
                       const std::string& thresholdDesc);

    /// Called when an alert closes — finds the matching ACTIVE entry,
    /// marks it CLOSED with final duration.
    void onAlertClosed(const AlertTracker::AlertEvent& event);

signals:
    void countChanged();

private:
    static QString formatDuration(qint64 seconds);

    struct Entry {
        QString  time;
        QString  sensor;
        double   value    {0.0};
        QString  unit;
        int      level    {0};
        QString  threshold;
        bool     active   {true};
        QDateTime openTime;
        qint64   durationSec{0};
    };

    std::deque<Entry> m_entries;
    QTimer*           m_ticker{nullptr}; // 1s tick to update live durations
};

} // namespace nmea