#include "DeviceGroupModel.h"
#include "ApiService.h"
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
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
                    group.name = obj["name"].toString();
                    group.timingPrice = obj["timingPrice"].toInt();
                    group.deviceCount = obj["deviceCount"].toInt();
                    group.bitrate = obj["bitrate"].toDouble();

                    QJsonArray charter = obj["charterFlightCost"].toArray();
                    for (const auto& price : charter) {
                        group.charterPrices.append(price.toInt());
                    }
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
