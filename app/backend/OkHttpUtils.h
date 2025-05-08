#ifndef OKHTTPUTILS_H
#define OKHTTPUTILS_H
#pragma once

#include <QObject>
#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QUrlQuery>
#include <QJsonDocument>
#include <QJsonObject>
#include <QSemaphore>
#include <QTimer>
#include <QMap>


class OkHttpUtils : public QObject
{
    Q_OBJECT

public:
    // 创建 builder
    static OkHttpUtils* builder(QObject* parent = nullptr);

    // 设置 URL
    OkHttpUtils* url(const QString& path);

    // 添加参数
    OkHttpUtils* addParam(const QString& key, const QString& value);

    // 添加 Header
    OkHttpUtils* addHeader(const QString& key, const QString& value);

    // GET 请求
    OkHttpUtils* get();

    // POST 请求（isJsonPost = true 表示 JSON 提交）
    OkHttpUtils* post(bool isJsonPost);

    // 同步请求
    QString sync();

    // 异步请求（返回结果）
    void async(std::function<void(QString)> onSuccess, std::function<void(QString)> onFailure);

private:
    explicit OkHttpUtils(QObject* parent = nullptr);

    QNetworkAccessManager m_manager;
    QNetworkRequest m_request;

    QMap<QString, QString> m_headers;
    QMap<QString, QString> m_params;

    QString m_url;
    bool m_isPost = false;
    bool m_isJsonPost = false;

    QByteArray buildRequestBody();
};


#endif // OKHTTPUTILS_H
