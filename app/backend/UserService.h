#ifndef USERSERVICE_H
#define USERSERVICE_H
#pragma once

#include <QObject>
#include <QString>

class UserService : public QObject {
    Q_OBJECT

public:
    explicit UserService(QObject* parent = nullptr);

    Q_INVOKABLE void login(const QString& username, const QString& password);
    Q_INVOKABLE void registerUser(const QString& username, const QString& password, const QString& confirmPwd);

    Q_INVOKABLE void updateUserInfo(const QString& field, const QString& value);

    // Fetches the user's coin balance. User ID is taken from the session
    // like other service calls such as getAllDeviceOrderInfoByUserId.
    Q_INVOKABLE void getUserInfoById();
    Q_INVOKABLE void getLatestNotice();


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

};

#endif // USERSERVICE_H
