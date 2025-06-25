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

signals:
    void loginSuccess(QString token);      // 简化处理，成功信号（你也可以改成 user 对象）
    void loginFailure(QString errorMsg);

    void registerSuccess(QString msg);
    void registerFailure(QString errorMsg);

    void updateUserInfoSuccess(QString msg);
    void updateUserInfoFailure(QString errorMsg);
};

#endif // USERSERVICE_H
