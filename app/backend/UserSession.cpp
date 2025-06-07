#include "UserSession.h"
#include <QSettings>

UserSession::UserSession(QObject* parent) : QObject(parent) {}

UserSession* UserSession::instance() {
    static UserSession* _instance = new UserSession();
    return _instance;
}

QString UserSession::username() const { return m_username; }
QString UserSession::token() const { return m_token; }

void UserSession::setUsername(const QString& name) {
    if (m_username != name) {
        m_username = name;
        emit usernameChanged();
    }
}

void UserSession::setToken(const QString& tkn) {
    if (m_token != tkn) {
        m_token = tkn;
        emit tokenChanged();
    }
}

void UserSession::saveToSettings() {
    QSettings settings("YourCompany", "Moonlight");
    settings.setValue("username", m_username);
    settings.setValue("token", m_token);
}

bool UserSession::restoreFromSettings() {
    QSettings settings("YourCompany", "Moonlight");
    QString savedUsername = settings.value("username").toString();
    QString savedToken = settings.value("token").toString();
    if (!savedToken.isEmpty()) {
        setUsername(savedUsername);
        setToken(savedToken);
        return true;
    }
    return false;
}

void UserSession::logout() {
    setUsername("");
    setToken("");

    QSettings settings("YourCompany", "Moonlight");
    settings.remove("username");
    settings.remove("token");
}

void UserSession::setUserId(qint64 id) {
    if (m_userId != id) {
        m_userId = id;
        emit userIdChanged();
    }
}
