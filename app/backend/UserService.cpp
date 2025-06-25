#include "UserService.h"
#include "UserSession.h"
#include "ApiService.h"
#include <QJsonDocument>
#include <QJsonObject>
#include <QDebug>

UserService::UserService(QObject* parent)
    : QObject(parent) {}

void UserService::login(const QString& username, const QString& password) {
    ApiService::login(username, password,
            [=](QString data) {
                QJsonDocument doc = QJsonDocument::fromJson(data.toUtf8());
                if (!doc.isNull() && doc.isObject()) {
                    QJsonObject obj = doc.object();
                    int code = obj.value("code").toInt();
                    if (code == 200) {
                        QJsonObject userData = obj.value("data").toObject();
                        QString token = userData.value("token").toString();
                        QString user = userData.value("username").toString();
                        qint64  userId = userData.value("userId").toVariant().toLongLong();

                        // ✅ 保存到全局 session
                        UserSession::instance()->setToken(token);
                        UserSession::instance()->setUsername(user);
                        UserSession::instance()->setUserId(userId);
                        UserSession::instance()->saveToSettings();  // 持久化保存

                        emit loginSuccess(token);
                    } else {
                        emit loginFailure(obj.value("message").toString());
                    }
                } else {
                    emit loginFailure("响应格式错误");
                }
            },
            [=](QString err) {
                emit loginFailure("网络错误: " + err);
            }
            );
}

void UserService::registerUser(const QString& username, const QString& password, const QString& confirmPwd) {
    if (password != confirmPwd) {
        emit registerFailure("两次密码不一致");
        return;
    }

    ApiService::registerUser(username, password, confirmPwd,
            [=](QString data) {
                QJsonDocument doc = QJsonDocument::fromJson(data.toUtf8());
                if (!doc.isNull() && doc.isObject()) {
                    QJsonObject obj = doc.object();
                    int code = obj.value("code").toInt();
                    if (code == 200) {
                        emit registerSuccess("注册成功");
                    } else {
                        emit registerFailure(obj.value("message").toString());
                    }
                } else {
                    emit registerFailure("响应格式错误");
                }
            },
            [=](QString err) {
                emit registerFailure("网络错误: " + err);
            }
            );
}


void UserService::updateUserInfo(const QString& field, const QString& value)
{
    QJsonObject obj;
    obj.insert("userId", QString::number(UserSession::instance()->userId()));
    obj.insert(field, value);

    ApiService::updateUserInfo(obj,
            [=](QString data) {
                QJsonDocument doc = QJsonDocument::fromJson(data.toUtf8());
                if (!doc.isNull() && doc.isObject()) {
                    QJsonObject res = doc.object();
                    int code = res.value("code").toInt();
                    if (code == 200) {
                        emit updateUserInfoSuccess(res.value("message").toString());
                    } else {
                        emit updateUserInfoFailure(res.value("message").toString());
                    }
                } else {
                    emit updateUserInfoFailure("响应格式错误");
                }
            },
            [=](QString err) {
                emit updateUserInfoFailure("网络错误: " + err);
            });
}

void UserService::getUserInfoById()
{
    qint64 uid = UserSession::instance()->userId();
    if (uid == 0) {
        emit userInfoFailure(QStringLiteral("用户未登录"));
        return;
    }

    ApiService::getUserInfoById(QString::number(uid),
            [=](QString data) {
                QJsonDocument doc = QJsonDocument::fromJson(data.toUtf8());
                if (!doc.isNull() && doc.isObject()) {
                    QJsonObject obj = doc.object();
                    int code = obj.value("code").toInt();
                    if (code == 200) {
                        QJsonObject userData = obj.value("data").toObject();
                        double balance = userData.value("balance").toDouble();
                        UserSession::instance()->setBalance(balance);
                        emit userInfoSuccess(balance);
                    } else {
                        emit userInfoFailure(obj.value("message").toString());
                    }
                } else {
                    emit userInfoFailure("响应格式错误");
                }
            },
            [=](QString err) {
                emit userInfoFailure("网络错误: " + err);
            }
            );
}

void UserService::getLatestNotice()
{
    ApiService::getLatestNotice(
            [=](QString data) {
                QJsonDocument doc = QJsonDocument::fromJson(data.toUtf8());
                if (!doc.isNull() && doc.isObject()) {
                    QJsonObject obj = doc.object();
                    int code = obj.value("code").toInt();
                    if (code == 200) {
                        QString notice = obj.value("data").toString();
                        UserSession::instance()->setNotice(notice);
                        emit noticeSuccess(notice);
                    } else {
                        emit noticeFailure(obj.value("message").toString());
                    }
                } else {
                    emit noticeFailure("响应格式错误");
                }
            },
            [=](QString err) {
                emit noticeFailure("网络错误: " + err);
            }
            );

}
