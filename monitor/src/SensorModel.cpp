#include "SensorModel.hpp"
#include "nmea/SensorFrame.hpp"

#include <QMetaObject>
#include <QThread>

namespace nmea {

// ══════════════════════════════════════════════════════════════════════════
// SensorModel
// ══════════════════════════════════════════════════════════════════════════

SensorModel::SensorModel(QObject* parent) : QAbstractListModel{parent} {}

void SensorModel::setConfigs(
    const std::unordered_map<uint8_t, SensorConfig>& configs)
{
    m_configs = configs;

    // Pre-populate one row per configured sensor so the table shows
    // all channels immediately — even those with no data yet.
    // Rows are inserted in id order for a consistent display order.
    std::vector<uint8_t> ids;
    ids.reserve(configs.size());
    for (const auto& [id, _] : configs) ids.push_back(id);
    std::sort(ids.begin(), ids.end());

    beginResetModel();
    m_rows.clear();
    for (const uint8_t id : ids) {
        const auto& cfg = configs.at(id);
        Row row;
        row.sensorName = QString::fromStdString(cfg.name);
        row.value      = 0.0;
        row.unit       = QString::fromStdString(cfg.unit);
        row.alertLevel = 0;
        row.timestamp  = "—";
        row.warnLow    = cfg.threshold.warningLow;
        row.warnHigh   = cfg.threshold.warningHigh;
        row.critLow    = cfg.threshold.criticalLow;
        row.critHigh   = cfg.threshold.criticalHigh;
        row.hasData    = false;
        m_rows.push_back(std::move(row));
    }
    endResetModel();
    emit countChanged();
}

int SensorModel::rowCount(const QModelIndex& parent) const
{
    if (parent.isValid()) return 0;
    return static_cast<int>(m_rows.size());
}

QVariant SensorModel::data(const QModelIndex& index, int role) const
{
    if (!index.isValid() || index.row() >= static_cast<int>(m_rows.size()))
        return {};
    const Row& r = m_rows[static_cast<std::size_t>(index.row())];
    switch (role) {
        case SensorNameRole:  return r.sensorName;
        case ValueRole:       return r.value;
        case UnitRole:        return r.unit;
        case AlertLevelRole:  return r.alertLevel;
        case TimestampRole:   return r.timestamp;
        case WarnLowRole:     return r.warnLow;
        case WarnHighRole:    return r.warnHigh;
        case CritLowRole:     return r.critLow;
        case CritHighRole:    return r.critHigh;
        case HasDataRole:     return r.hasData;
        default:              return {};
    }
}

QHash<int, QByteArray> SensorModel::roleNames() const
{
    return {
        {SensorNameRole, "sensorName"},
        {ValueRole,      "value"},
        {UnitRole,       "unit"},
        {AlertLevelRole, "alertLevel"},
        {TimestampRole,  "timestamp"},
        {WarnLowRole,    "warnLow"},
        {WarnHighRole,   "warnHigh"},
        {CritLowRole,    "critLow"},
        {CritHighRole,   "critHigh"},
        {HasDataRole,    "hasData"},
    };
}

void SensorModel::pushFrame(SensorFrame frame)
{
    if (QThread::currentThread() != thread()) {
        QMetaObject::invokeMethod(this, [this, f = std::move(frame)]() mutable {
            pushFrameOnMainThread(std::move(f));
        }, Qt::QueuedConnection);
        return;
    }
    pushFrameOnMainThread(std::move(frame));
}

void SensorModel::pushFrameOnMainThread(SensorFrame frame)
{
    const uint8_t id = static_cast<uint8_t>(frame.sensorId);
    Row row;
    if (auto it = m_configs.find(id); it != m_configs.end())
        row.sensorName = QString::fromStdString(it->second.name);
    else
        row.sensorName = QString("Sensor 0x%1").arg(id, 2, 16, QChar('0'));
    row.value      = frame.value;
    row.unit       = QString::fromStdString(frame.unit);
    row.alertLevel = static_cast<int>(frame.alertLevel);
    row.timestamp  = QDateTime::currentDateTime().toString("yyyy-MM-ddTHH:mm:ss");

    if (auto it = m_configs.find(id); it != m_configs.end()) {
        row.warnLow  = it->second.threshold.warningLow;
        row.warnHigh = it->second.threshold.warningHigh;
        row.critLow  = it->second.threshold.criticalLow;
        row.critHigh = it->second.threshold.criticalHigh;
    }

    row.hasData = true;

    if (frame.alertLevel == AlertLevel::Critical)
        emit criticalAlert(row.sensorName, row.value, row.unit);

    // All rows pre-populated from config in setConfigs() — just update.
    for (int i = 0; i < static_cast<int>(m_rows.size()); ++i) {
        if (m_rows[i].sensorName == row.sensorName) {
            m_rows[i] = std::move(row);
            emit dataChanged(index(i), index(i),
                {ValueRole, AlertLevelRole, TimestampRole, HasDataRole});
            recomputeAlertCounts();
            return;
        }
    }

    const int pos = static_cast<int>(m_rows.size());
    beginInsertRows({}, pos, pos);
    m_rows.push_back(std::move(row));
    endInsertRows();
    recomputeAlertCounts();
    emit countChanged();
}

void SensorModel::recomputeAlertCounts()
{
    int critical = 0, warning = 0;
    for (const auto& r : m_rows) {
        if      (r.alertLevel == 2) ++critical;
        else if (r.alertLevel == 1) ++warning;
    }
    bool changed = false;
    if (critical != m_criticalCount) { m_criticalCount = critical; changed = true; }
    if (warning  != m_warningCount)  { m_warningCount  = warning;  changed = true; }
    if (changed) emit alertCountChanged();
}

// ══════════════════════════════════════════════════════════════════════════
// EventLogModel
// ══════════════════════════════════════════════════════════════════════════

EventLogModel::EventLogModel(QObject* parent)
    : QAbstractListModel{parent}
{
    // 1-second ticker to update live durations of ACTIVE entries
    m_ticker = new QTimer(this);
    m_ticker->setInterval(1000);
    connect(m_ticker, &QTimer::timeout, this, [this] {
        for (int i = 0; i < static_cast<int>(m_entries.size()); ++i) {
            if (m_entries[i].active) {
                m_entries[i].durationSec =
                    m_entries[i].openTime.secsTo(QDateTime::currentDateTime());
                emit dataChanged(index(i), index(i), {EventDurationRole});
            }
        }
    });
    m_ticker->start();
}

int EventLogModel::rowCount(const QModelIndex& parent) const
{
    if (parent.isValid()) return 0;
    return static_cast<int>(m_entries.size());
}

QVariant EventLogModel::data(const QModelIndex& idx, int role) const
{
    if (!idx.isValid() || idx.row() >= static_cast<int>(m_entries.size()))
        return {};
    const Entry& e = m_entries[static_cast<std::size_t>(idx.row())];
    switch (role) {
        case EventTimeRole:     return e.time;
        case EventSensorRole:   return e.sensor;
        case EventValueRole:    return e.value;
        case EventUnitRole:     return e.unit;
        case EventLevelRole:    return e.level;
        case EventThreshRole:   return e.threshold;
        case EventStatusRole:   return e.active ? QString("ACTIVE") : QString("CLOSED");
        case EventDurationRole: return formatDuration(e.durationSec);
        default:                return {};
    }
}

QHash<int, QByteArray> EventLogModel::roleNames() const
{
    return {
        {EventTimeRole,     "eventTime"},
        {EventSensorRole,   "eventSensor"},
        {EventValueRole,    "eventValue"},
        {EventUnitRole,     "eventUnit"},
        {EventLevelRole,    "eventLevel"},
        {EventThreshRole,   "eventThresh"},
        {EventStatusRole,   "eventStatus"},
        {EventDurationRole, "eventDuration"},
    };
}

void EventLogModel::onAlertOpened(const AlertTracker::AlertEvent& event,
                                   const std::string& thresholdDesc)
{
    Entry e;
    e.time      = QDateTime::currentDateTime().toString("HH:mm:ss");
    e.sensor    = QString::fromStdString(event.sensorName);
    e.value     = event.openValue;
    e.unit      = QString::fromStdString(event.unit);
    e.level     = static_cast<int>(event.level);
    e.threshold = QString::fromStdString(thresholdDesc);
    e.active    = true;
    e.openTime  = QDateTime::currentDateTime();
    e.durationSec = 0;

    // Insert at front (newest first)
    beginInsertRows({}, 0, 0);
    m_entries.push_front(std::move(e));
    // Enforce max 50 entries
    if (static_cast<int>(m_entries.size()) > kMaxEvents) {
        beginRemoveRows({}, kMaxEvents, static_cast<int>(m_entries.size()) - 1);
        m_entries.resize(kMaxEvents);
        endRemoveRows();
    }
    endInsertRows();
    emit countChanged();
}

void EventLogModel::onAlertClosed(const AlertTracker::AlertEvent& event)
{
    const QString sensorName =
        QString::fromStdString(event.sensorName);

    // Find the matching ACTIVE entry and close it
    for (int i = 0; i < static_cast<int>(m_entries.size()); ++i) {
        Entry& e = m_entries[i];
        if (e.active && e.sensor == sensorName) {
            e.active      = false;
            e.durationSec = e.openTime.secsTo(QDateTime::currentDateTime());
            emit dataChanged(index(i), index(i),
                {EventStatusRole, EventDurationRole});
            return;
        }
    }
}

QString EventLogModel::formatDuration(qint64 seconds)
{
    const qint64 h   = seconds / 3600;
    const qint64 m   = (seconds % 3600) / 60;
    const qint64 s   = seconds % 60;
    return QString("%1:%2:%3")
        .arg(h, 2, 10, QChar('0'))
        .arg(m, 2, 10, QChar('0'))
        .arg(s, 2, 10, QChar('0'));
}

} // namespace nmea