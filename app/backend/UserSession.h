#ifndef USERSESSION_H
#define USERSESSION_H
#pragma once

#include <QObject>

class UserSession : public QObject {
    Q_OBJECT
    Q_PROPERTY(QString username READ username WRITE setUsername NOTIFY usernameChanged)
    Q_PROPERTY(QString token READ token WRITE setToken NOTIFY tokenChanged)
    Q_PROPERTY(qint64  userId READ userId WRITE setUserId NOTIFY userIdChanged)
    Q_PROPERTY(int balance READ balance WRITE setBalance NOTIFY balanceChanged)
    Q_PROPERTY(QString notice READ notice WRITE setNotice NOTIFY noticeChanged)

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
    qint64  userId() const { return m_userId; }
    void    setUserId(qint64 id);
    int     balance() const { return m_balance; }
    void    setBalance(int balance);
    QString notice() const { return m_notice; }
    void    setNotice(const QString& notice);

signals:
    void usernameChanged();
    void tokenChanged();
    void userIdChanged();
    void balanceChanged();
    void noticeChanged();

private:
    explicit UserSession(QObject* parent = nullptr);
    QString m_username;
    QString m_token;
    qint64  m_userId = 0;
    int     m_balance = 0;
    QString m_notice;
};

#endif // USERSESSION_H
