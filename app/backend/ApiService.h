#ifndef APISERVICE_H
#define APISERVICE_H
#pragma once

#include <QObject>
#include <QString>
#include <functional>

namespace ApiService {

struct PinRequest {
    QString localIP;
    QString port;
    QString name;
    QString pin;
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

void getAllDeviceOrderInfoByUserId(const QString& userId,
                                   std::function<void(QString)> onSuccess,
                                   std::function<void(QString)> onFailure);

void allocateDevice(const QString& userId, const QString& deviceGroupId, const QString& billingType,
                    const QString& requestId,
                    std::function<void(QString)> onSuccess,
                    std::function<void(QString)> onFailure);

}

#endif // APISERVICE_H
