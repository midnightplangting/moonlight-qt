#include "DeviceGroupModel.h"
#include "ApiService.h"
#include "Logger.h"
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QVariantMap>
#include <QDebug>

DeviceGroupModel::DeviceGroupModel(QObject* parent)
    : QAbstractListModel(parent) {}

int DeviceGroupModel::rowCount(const QModelIndex&) const {
    return m_data.size();
}

QVariant DeviceGroupModel::data(const QModelIndex& index, int role) const {
    if (!index.isValid() || index.row() >= m_data.size())
        return {};

    const DeviceGroup& item = m_data[index.row()];

    switch (role) {
    case NameRole: return item.name;
    case HourlyRole: return item.timingPrice;
    case DayRole: return item.charterPrices.value(0, 0);
    case WeekRole: return item.charterPrices.value(1, 0);
    case MonthRole: return item.charterPrices.value(2, 0);
    case DeviceCountRole: return item.deviceCount;
    case BitrateRole: return item.bitrate;
    case GroupIdRole: return item.groupId;

    }
    return {};
}

QHash<int, QByteArray> DeviceGroupModel::roleNames() const {
    return {
        { NameRole, "name" },
        { HourlyRole, "hourly" },
        { DayRole, "day" },
        { WeekRole, "week" },
        { MonthRole, "month" },
        { DeviceCountRole, "deviceCount" },
        { BitrateRole, "bitrate" },
        { GroupIdRole, "groupId" }
    };
}

void DeviceGroupModel::setDeviceGroups(const QVector<DeviceGroup>& list) {
    beginResetModel();
    m_data = list;
    endResetModel();
}

void DeviceGroupModel::refresh()
{
    ApiService::getDeviceGroupList([
        this
    ](QString data) {
            QVector<DeviceGroup> parsed;
            QJsonDocument doc = QJsonDocument::fromJson(data.toUtf8());
            if (doc.isObject()) {
                QJsonArray array = doc["data"].toArray();
                for (const auto& val : array) {
                    QJsonObject obj = val.toObject();
                    DeviceGroup group;
                    group.groupId = obj["deviceGroupId"].toInt();
                    group.name = obj["name"].toString();
                    group.timingPrice = obj["timingPrice"].toInt();
                    group.deviceCount = obj["deviceCount"].toInt();
                    group.bitrate = obj["bitrate"].toDouble();

                    QJsonArray charter = obj["charterFlightCost"].toArray();
                    for (const auto& price : charter) {
                        group.charterPrices.append(price.toInt());
                    }
                    LOG_INFO(QStringLiteral("[DeviceGroup] id=%1 name=%2 timing=%3 charter=[%4,%5,%6]")
                                 .arg(group.groupId)
                                 .arg(group.name)
                                 .arg(group.timingPrice)
                                 .arg(group.charterPrices.value(0))
                                 .arg(group.charterPrices.value(1))
                                 .arg(group.charterPrices.value(2)));
                    parsed.append(group);
                }
            }

            setDeviceGroups(parsed);
        }, [
            this
        ](QString err) {
            qWarning() << "Failed to fetch device group list:" << err;
        });
}

QVariantMap DeviceGroupModel::getGroup(int groupId) const
{
    QVariantMap map;
    for (const auto& g : m_data) {
        if (g.groupId == groupId) {
            map.insert("hourly", g.timingPrice);
            map.insert("day", g.charterPrices.value(0));
            map.insert("week", g.charterPrices.value(1));
            map.insert("month", g.charterPrices.value(2));
            return map;
        }
    }
    return map;
}
