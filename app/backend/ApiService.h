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
                  const QString& email, const QString& phone,
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

void rechargeOrder(const QString& orderId,
                   const QString& num,
                   const QString& billingType,
                   std::function<void(QString)> onSuccess,
                   std::function<void(QString)> onFailure);

void getGoldCoinPriceList(std::function<void(QString)> onSuccess,
                          std::function<void(QString)> onFailure);

void payPC(const QString& token,
            const QString& userId,
            const QString& goldCoinPriceId,
            std::function<void(QString)> onSuccess,
            std::function<void(QString)> onFailure);



void updateUserInfo(const QJsonObject& params,
                    std::function<void(QString)> onSuccess,
                    std::function<void(QString)> onFailure);

void getOrderDetailList(const QString& userId,
                        std::function<void(QString)> onSuccess,
                        std::function<void(QString)> onFailure);

void getUserInfoById(const QString& userId,
                     std::function<void(QString)> onSuccess,
                     std::function<void(QString)> onFailure);

void getLatestNotice(std::function<void(QString)> onSuccess,
                     std::function<void(QString)> onFailure);

void exchangeCoupon(const QString& userId,
                    const QString& discountCode,
                    const QString& requestId,
                    std::function<void(QString)> onSuccess,
                    std::function<void(QString)> onFailure);

void restartSunshine(const QString& orderId,
                     std::function<void(QString)> onSuccess,
                     std::function<void(QString)> onFailure);

void checkLatestVersion(std::function<void(QString)> onSuccess,
                        std::function<void(QString)> onFailure);


}

#endif // APISERVICE_H
