#ifndef DEVICEGROUPMODEL_H
#define DEVICEGROUPMODEL_H
#pragma once

#include <QAbstractListModel>
#include <QVector>

struct DeviceGroup {
    int groupId;
    QString name;
    int timingPrice;
    QList<int> charterPrices;  // [day, week, month]
    int deviceCount;
    double bitrate;
};


class DeviceGroupModel : public QAbstractListModel {
    Q_OBJECT

public:
    enum Roles {
        NameRole = Qt::UserRole + 1,
        HourlyRole,
        DayRole,
        WeekRole,
        MonthRole,
        DeviceCountRole,
        BitrateRole,
        GroupIdRole
    };


    DeviceGroupModel(QObject* parent = nullptr);

    int rowCount(const QModelIndex& parent = QModelIndex()) const override;
    QVariant data(const QModelIndex& index, int role) const override;
    QHash<int, QByteArray> roleNames() const override;

    void setDeviceGroups(const QVector<DeviceGroup>& list);

    Q_INVOKABLE void refresh();

    Q_INVOKABLE QVariantMap getGroup(int groupId) const;

private:
    QVector<DeviceGroup> m_data;
};

#endif // DEVICEGROUPMODEL_H
