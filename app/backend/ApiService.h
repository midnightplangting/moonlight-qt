#ifndef APISERVICE_H
#define APISERVICE_H
#pragma once

#include <QObject>
#include <QString>
#include <functional>
#include <QJsonObject>

namespace ApiService {

struct PinRequest {
    qint64 orderId = 0;
    QString localIP;
    QString port;
    QString name;
    QString pinStr;
};

void sendPin(const PinRequest& req,
             std::function<void(bool)> onSuccess,
             std::function<void(QString)> onFailure);

void login(const QString& username, const QString& password,
           std::function<void(QString)> onSuccess,
           std::function<void(QString)> onFailure);

void registerUser(const QString& username, const QString& password, const QString& confirmPwd,
                  std::function<void(QString)> onSuccess,
                  std::function<void(QString)> onFailure);

void getDeviceGroupList(std::function<void(QString)> onSuccess,
                        std::function<void(QString)> onFailure);

QString getDeviceGroupListSync();

QString getAllDeviceOrderInfoByUserIdSync(const QString& userId);

void getAllDeviceOrderInfoByUserId(const QString& userId,
                                   std::function<void(QString)> onSuccess,
                                   std::function<void(QString)> onFailure);

void allocateDevice(const QString& userId, const QString& deviceGroupId, const QString& billingType,
                    const QString& requestId,
                    std::function<void(QString)> onSuccess,
                    std::function<void(QString)> onFailure);

void closeOrder(const QString& orderId,
                std::function<void(QString)> onSuccess,
                std::function<void(QString)> onFailure);

void updateUserInfo(const QJsonObject& params,
                    std::function<void(QString)> onSuccess,
                    std::function<void(QString)> onFailure);

}

#endif // APISERVICE_H
