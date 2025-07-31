#ifndef USERSERVICE_H
#define USERSERVICE_H
#pragma once

#include <QObject>
#include <QString>
#include <QVariantList>
#include <QNetworkAccessManager>

class UserService : public QObject {
    Q_OBJECT

public:
    explicit UserService(QObject* parent = nullptr);

    Q_INVOKABLE void login(const QString& username, const QString& password);
    Q_INVOKABLE void registerUser(const QString& username, const QString& password,
                                 const QString& confirmPwd, const QString& email,
                                 const QString& phone);

    Q_INVOKABLE void updateUserInfo(const QString& field, const QString& value);

    // Fetches the user's coin balance. User ID is taken from the session
    // like other service calls such as getAllDeviceOrderInfoByUserId.
    Q_INVOKABLE void getUserInfoById();
    Q_INVOKABLE void getLatestNotice();
    Q_INVOKABLE void exchangeCoupon(const QString& discountCode);
    Q_INVOKABLE void getGoldCoinPriceList();
    Q_INVOKABLE void payWxPC(int goldCoinPriceId);
    Q_INVOKABLE void payAliPC(int goldCoinPriceId);
    Q_INVOKABLE void checkForUpdate();
    Q_INVOKABLE void downloadUpdate(const QString& url);
    Q_INVOKABLE void installUpdate(const QString& filePath);


signals:
    void loginSuccess(QString token);      // 简化处理，成功信号（你也可以改成 user 对象）
    void loginFailure(QString errorMsg);

    void registerSuccess(QString msg);
    void registerFailure(QString errorMsg);


    void updateUserInfoSuccess(QString msg);
    void updateUserInfoFailure(QString errorMsg);

    void userInfoSuccess(double balance);
    void userInfoFailure(QString errorMsg);
    void noticeSuccess(QString notice);
    void noticeFailure(QString errorMsg);

    void exchangeCouponSuccess(QString message);
    void exchangeCouponFailure(QString errorMsg);

    void goldCoinPriceListSuccess(QVariantList list);
    void goldCoinPriceListFailure(QString errorMsg);

    void updateAvailable(QString content, QString url);
    void updateDownloadProgress(qreal progress);
    void updateDownloadFinished(QString filePath);

    void payPCSuccess(QString qrData);
    void payPCFailure(QString errorMsg);

private:
    QNetworkAccessManager m_updateManager;
};

#endif // USERSERVICE_H
