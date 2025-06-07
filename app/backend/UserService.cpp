#include "UserService.h"
#include "UserSession.h"
#include "OkHttpUtils.h"
#include <QJsonDocument>
#include <QJsonObject>
#include <QDebug>

UserService::UserService(QObject* parent)
    : QObject(parent) {}

void UserService::login(const QString& username, const QString& password) {
    OkHttpUtils::builder()
    ->url("user/login")
        ->addParam("type", "1")  // ⚠️ 改为 0 表示用户名密码登录方式
        ->addParam("username", username)
        ->addParam("password", password)
        ->post(true)
        ->async(
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

    OkHttpUtils::builder()
        ->url("user/register")
        ->addParam("username", username)
        ->addParam("password", password)
        ->addParam("email", username + "@default.com")  // 先默认填充
        ->addParam("phone", "00000000000")
        ->addParam("role", "0")
        ->post(true)
        ->async(
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
