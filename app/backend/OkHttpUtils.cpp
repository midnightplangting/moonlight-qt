#include "OkHttpUtils.h"
#include "UserSession.h"
#include "Logger.h"
#include <QEventLoop>
#include <QUrl>
#include <QUrlQuery>
#include <QJsonDocument>
#include <QJsonObject>
#include <QDebug>

#define BASE_URL "https://gzydn.cn:18081/"

OkHttpUtils* OkHttpUtils::builder(QObject* parent) {
    return new OkHttpUtils(parent);
}

OkHttpUtils::OkHttpUtils(QObject* parent)
    : QObject(parent) {}

OkHttpUtils* OkHttpUtils::url(const QString& path) {
    m_url = BASE_URL + path;
    return this;
}

OkHttpUtils* OkHttpUtils::addParam(const QString& key, const QString& value) {
    m_params.insert(key, value);
    return this;
}

OkHttpUtils* OkHttpUtils::addHeader(const QString& key, const QString& value) {
    m_headers.insert(key, value);
    return this;
}

OkHttpUtils* OkHttpUtils::get() {
    m_isPost = false;
    return this;
}

OkHttpUtils* OkHttpUtils::post(bool isJsonPost) {
    m_isPost = true;
    m_isJsonPost = isJsonPost;
    return this;
}

QByteArray OkHttpUtils::buildRequestBody() {
    if (m_isJsonPost) {
        QJsonObject jsonObj;
        for (auto it = m_params.begin(); it != m_params.end(); ++it) {
            jsonObj.insert(it.key(), it.value());
        }
        return QJsonDocument(jsonObj).toJson();
    } else {
        QUrlQuery formData;
        for (auto it = m_params.begin(); it != m_params.end(); ++it) {
            formData.addQueryItem(it.key(), it.value());
        }
        return formData.toString(QUrl::FullyEncoded).toUtf8();
    }
}

QString OkHttpUtils::sync() {
    QNetworkReply* reply = nullptr;

    QUrl fullUrl = QUrl(m_url);
    if (!m_isPost && !m_params.isEmpty()) {
        QUrlQuery query;
        for (auto it = m_params.begin(); it != m_params.end(); ++it) {
            query.addQueryItem(it.key(), it.value());
        }
        fullUrl.setQuery(query);
    }
    m_request.setUrl(fullUrl);

    // 打印完整请求信息
    LOG_DEBUG(QStringLiteral("[OkHttpUtils] URL: %1").arg(m_request.url().toString()));
    LOG_DEBUG(QStringLiteral("[OkHttpUtils] Method: %1").arg(m_isPost ? "POST" : "GET"));
    LOG_DEBUG(QStringLiteral("[OkHttpUtils] Headers:"));
    for (const auto& key : m_request.rawHeaderList()) {
        LOG_DEBUG(QStringLiteral("    %1: %2")
                       .arg(QString::fromUtf8(key))
                       .arg(QString::fromUtf8(m_request.rawHeader(key))));
    }
    if (m_isPost) {
        LOG_DEBUG(QStringLiteral("[OkHttpUtils] Body: %1").arg(QString::fromUtf8(buildRequestBody())));
    }

    // 设置 headers
    for (auto it = m_headers.begin(); it != m_headers.end(); ++it) {
        m_request.setRawHeader(it.key().toUtf8(), it.value().toUtf8());
    }

    // 自动附加 Authorization
    QString token = UserSession::instance()->token();
    LOG_DEBUG(QStringLiteral("[OkHttpUtils] token: %1").arg(token));
    if (!token.isEmpty()) {
        m_request.setRawHeader("Authorization", token.toUtf8());
    }

    // qDebug() << "[OkHttpUtils] token:" << token;

    if (m_isPost) {
        QByteArray body = buildRequestBody();
        m_request.setHeader(QNetworkRequest::ContentTypeHeader,
                            m_isJsonPost ? "application/json" : "application/x-www-form-urlencoded");
        reply = m_manager.post(m_request, body);
    } else {
        reply = m_manager.get(m_request);
    }

    QEventLoop loop;
    connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
    loop.exec();

    if (reply->error() != QNetworkReply::NoError) {
        QString error = reply->errorString();
        reply->deleteLater();
        return "请求失败：" + error;
    }

    QString result = reply->readAll();
    reply->deleteLater();
    return result;
}

void OkHttpUtils::async(std::function<void(QString)> onSuccess, std::function<void(QString)> onFailure) {
    QUrl fullUrl = QUrl(m_url);
    if (!m_isPost && !m_params.isEmpty()) {
        QUrlQuery query;
        for (auto it = m_params.begin(); it != m_params.end(); ++it) {
            query.addQueryItem(it.key(), it.value());
        }
        fullUrl.setQuery(query);
    }
    m_request.setUrl(fullUrl);

    // 打印完整请求信息
    LOG_DEBUG(QStringLiteral("[OkHttpUtils] URL: %1").arg(m_request.url().toString()));
    LOG_DEBUG(QStringLiteral("[OkHttpUtils] Method: %1").arg(m_isPost ? "POST" : "GET"));
    LOG_DEBUG(QStringLiteral("[OkHttpUtils] Headers:"));
    for (const auto& key : m_request.rawHeaderList()) {
        LOG_DEBUG(QStringLiteral("    %1: %2")
                       .arg(QString::fromUtf8(key))
                       .arg(QString::fromUtf8(m_request.rawHeader(key))));
    }
    if (m_isPost) {
        LOG_DEBUG(QStringLiteral("[OkHttpUtils] Body: %1").arg(QString::fromUtf8(buildRequestBody())));
    }

    // 设置 headers
    for (auto it = m_headers.begin(); it != m_headers.end(); ++it) {
        m_request.setRawHeader(it.key().toUtf8(), it.value().toUtf8());
    }

    // 自动附加 Authorization
    QString token = UserSession::instance()->token();
    LOG_DEBUG(QStringLiteral("[OkHttpUtils] token: %1").arg(token));
    if (!token.isEmpty()) {
        m_request.setRawHeader("Authorization", token.toUtf8());
    }

    QNetworkReply* reply = nullptr;
    if (m_isPost) {
        QByteArray body = buildRequestBody();
        m_request.setHeader(QNetworkRequest::ContentTypeHeader,
                            m_isJsonPost ? "application/json" : "application/x-www-form-urlencoded");
        reply = m_manager.post(m_request, body);
    } else {
        reply = m_manager.get(m_request);
    }

    connect(reply, &QNetworkReply::finished, this, [reply, onSuccess, onFailure]() {
        if (reply->error() != QNetworkReply::NoError) {
            onFailure(reply->errorString());
        } else {
            QString result = reply->readAll();
            onSuccess(result);
        }
        reply->deleteLater();
    });
}
