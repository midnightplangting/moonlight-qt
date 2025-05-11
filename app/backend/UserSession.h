#ifndef USERSESSION_H
#define USERSESSION_H
#pragma once

#include <QObject>

class UserSession : public QObject {
    Q_OBJECT
    Q_PROPERTY(QString username READ username WRITE setUsername NOTIFY usernameChanged)
    Q_PROPERTY(QString token READ token WRITE setToken NOTIFY tokenChanged)

public:
    static UserSession* instance();

    QString username() const;
    QString token() const;

    void setUsername(const QString& name);
    void setToken(const QString& tkn);

    // 👇 增加这几行 Q_INVOKABLE，让 QML 也能调用
    Q_INVOKABLE void logout();               // 主动登出
    Q_INVOKABLE bool restoreFromSettings();  // 启动时恢复 token + username
    Q_INVOKABLE void saveToSettings();       // 登录后保存 token + username


signals:
    void usernameChanged();
    void tokenChanged();

private:
    explicit UserSession(QObject* parent = nullptr);
    QString m_username;
    QString m_token;
};

#endif // USERSESSION_H
