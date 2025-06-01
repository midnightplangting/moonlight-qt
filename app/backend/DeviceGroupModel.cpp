#include "DeviceGroupModel.h"

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
        { BitrateRole, "bitrate" }
    };
}

void DeviceGroupModel::setDeviceGroups(const QVector<DeviceGroup>& list) {
    beginResetModel();
    m_data = list;
    endResetModel();
}
