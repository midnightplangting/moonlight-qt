#include "ApiService.h"
#include "OkHttpUtils.h"
#include "Logger.h"
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>

using namespace ApiService;

void ApiService::sendPin(const PinRequest& req,
                         std::function<void(bool)> onSuccess,
                         std::function<void(QString)> onFailure)
{
    OkHttpUtils::builder()
        ->url("device/sendPin")
        ->addParam("orderId", QString::number(req.orderId))
        ->addParam("localIP", req.localIP)
        ->addParam("port", req.port)
        ->addParam("name", req.name)
        ->addParam("pinStr", req.pinStr)
        ->post(true)
        ->async([
            onSuccess
        ](QString data) {
            bool result = false;
            QJsonDocument doc = QJsonDocument::fromJson(data.toUtf8());
            if (doc.isObject()) {
                QJsonObject obj = doc.object();
                int code = obj.value("code").toInt();
                if (code == 200) {
                    result = obj.value("data").toBool();
                }
            }
            if (onSuccess)
                onSuccess(result);
        }, [
            onFailure
        ](QString err) {
            if (onFailure)
                onFailure(err);
        });
}

void ApiService::login(const QString& username, const QString& password,
                       std::function<void(QString)> onSuccess,
                       std::function<void(QString)> onFailure)
{
    OkHttpUtils::builder()
        ->url("user/login")
        ->addParam("type", "1")
        ->addParam("username", username)
        ->addParam("password", password)
        ->post(true)
        ->async(onSuccess, onFailure);
}

void ApiService::restartSunshine(const QString& orderId,
                                 std::function<void(QString)> onSuccess,
                                 std::function<void(QString)> onFailure)
{
    OkHttpUtils::builder()
        ->url("device/reStartSunshine")
        ->addParam("orderId", orderId)
        ->post(false)
        ->async(onSuccess, onFailure);
}

void ApiService::registerUser(const QString& username, const QString& password, const QString& confirmPwd,
                              const QString& email, const QString& phone,
                              std::function<void(QString)> onSuccess,
                              std::function<void(QString)> onFailure)
{
    Q_UNUSED(confirmPwd);
    OkHttpUtils::builder()
        ->url("user/register")
        ->addParam("username", username)
        ->addParam("password", password)
        ->addParam("email", email)
        ->addParam("phone", phone)
        ->addParam("role", "0")
        ->post(true)
        ->async(onSuccess, onFailure);
}

void ApiService::getDeviceGroupList(std::function<void(QString)> onSuccess,
                                    std::function<void(QString)> onFailure)
{
    OkHttpUtils::builder()
        ->url("deviceGroup/getDeviceGroupList")
        ->get()
        ->async(onSuccess, onFailure);
}

QString ApiService::getDeviceGroupListSync()
{
    return OkHttpUtils::builder()
        ->url("deviceGroup/getDeviceGroupList")
        ->get()
        ->sync();
}

void ApiService::getAllDeviceOrderInfoByUserId(const QString& userId,
                                               std::function<void(QString)> onSuccess,
                                               std::function<void(QString)> onFailure)
{
    OkHttpUtils::builder()
        ->url("user/getAllDeviceOrderInfoByUserId")
        ->addParam("userId", userId)
        ->post(false)
        ->async(onSuccess, onFailure);
}

QString ApiService::getAllDeviceOrderInfoByUserIdSync(const QString& userId)
{
    return OkHttpUtils::builder()
        ->url("user/getAllDeviceOrderInfoByUserId")
        ->addParam("userId", userId)
        ->post(false)
        ->sync();
}

void ApiService::allocateDevice(const QString& userId, const QString& deviceGroupId, const QString& billingType,
                                const QString& requestId,
                                std::function<void(QString)> onSuccess,
                                std::function<void(QString)> onFailure)
{
    OkHttpUtils::builder()
        ->url("device/allocateDevice")
        ->addParam("userId", userId)
        ->addParam("deviceGroupId", deviceGroupId)
        ->addParam("billingType", billingType)
        ->addParam("requestId", requestId)
        ->post(true)
        ->async(onSuccess, onFailure);
}

void ApiService::closeOrder(const QString& orderId,
                            std::function<void(QString)> onSuccess,
                            std::function<void(QString)> onFailure)
{
    OkHttpUtils::builder()
        ->url("device/closeOrder")
        ->addParam("orderId", orderId)
        ->post(false)
        ->async(onSuccess, onFailure);
}

void ApiService::rechargeOrder(const QString& orderId,
                               const QString& num,
                               const QString& billingType,
                               std::function<void(QString)> onSuccess,
                               std::function<void(QString)> onFailure)
{
    OkHttpUtils::builder()
        ->url("device/reChargeOrder")
        ->addParam("orderId", orderId)
        ->addParam("num", num)
        ->addParam("billingType", billingType)
        ->post(false)
        ->async(onSuccess, onFailure);
}

void ApiService::getGoldCoinPriceList(std::function<void(QString)> onSuccess,
                                      std::function<void(QString)> onFailure)
{
    OkHttpUtils::builder()
        ->url("goldCoinPrice/getGoldCoinPriceList")
        ->get()
        ->async(onSuccess, onFailure);
}

void ApiService::payPC(const QString& token,
                       const QString& userId,
                       const QString& goldCoinPriceId,
                       std::function<void(QString)> onSuccess,
                       std::function<void(QString)> onFailure)
{
    QString url = QStringLiteral("wx/payPC?userId=%1&goldCoinPriceId=%2")
                       .arg(userId, goldCoinPriceId);
    LOG_DEBUG(QStringLiteral("[ApiService::payPC] url=%1 token=%2")
                      .arg(url)
                      .arg(token));

    OkHttpUtils::builder()
        ->url(url)
        ->addHeader("token", token)
        ->post(false)
        ->async(onSuccess, onFailure);
}

void ApiService::getDevicePriceList(const QString& token,
                                    const QString& deviceId,
                                    std::function<void(QString)> onSuccess,
                                    std::function<void(QString)> onFailure)
{
    QString url = QStringLiteral("deviceGroup/getDevicePriceList?deviceId=%1").arg(deviceId);
    OkHttpUtils::builder()
        ->url(url)
        ->addHeader("token", token)
        ->get()
        ->async(onSuccess, onFailure);
}


void ApiService::updateUserInfo(const QJsonObject& params,
                                std::function<void(QString)> onSuccess,
                                std::function<void(QString)> onFailure)
{
    OkHttpUtils* builder = OkHttpUtils::builder();
    builder->url("user/updateUserInfo");
    for (auto it = params.begin(); it != params.end(); ++it) {
        builder->addParam(it.key(), it.value().toString());
    }
    builder->post(true)->async(onSuccess, onFailure);
}
void ApiService::getOrderDetailList(const QString& userId,
                                    std::function<void(QString)> onSuccess,
                                    std::function<void(QString)> onFailure)
{
    OkHttpUtils::builder()
        ->url("order/getOrderDetailList")
        ->addParam("userId", userId)
        ->get()
        ->async(onSuccess, onFailure);
}

void ApiService::getUserInfoById(const QString& userId,
                                 std::function<void(QString)> onSuccess,
                                 std::function<void(QString)> onFailure)
{
    // userId is sent as a parameter in a POST request
    OkHttpUtils::builder()
        ->url("user/getUserInfoById")
        ->addParam("userId", userId)
        ->post(false)
        ->async(onSuccess, onFailure);
}

void ApiService::getLatestNotice(std::function<void(QString)> onSuccess,
                                 std::function<void(QString)> onFailure)
{
    OkHttpUtils::builder()
        ->url("notice/getLatestNotice")
        ->get()
        ->async(onSuccess, onFailure);

}

void ApiService::exchangeCoupon(const QString& userId, const QString& discountCode,
                                const QString& requestId,
                                std::function<void(QString)> onSuccess,
                                std::function<void(QString)> onFailure)
{
    OkHttpUtils::builder()
        ->url("coupon/exchangeCoupon")
        ->addParam("userId", userId)
        ->addParam("discountCode", discountCode)
        ->addParam("requestId", requestId)
        ->post(false)
        ->async(onSuccess, onFailure);
}

void ApiService::checkLatestVersion(std::function<void(QString)> onSuccess,
                                    std::function<void(QString)> onFailure)
{
    QNetworkRequest request(QUrl("https://gzydn.cn:18081/pc/getLastestVersion"));
    QNetworkAccessManager* nam = new QNetworkAccessManager();

    QNetworkReply* reply = nam->get(request);
    QObject::connect(reply, &QNetworkReply::finished, [=]() {
        nam->deleteLater();
        if (reply->error() != QNetworkReply::NoError) {
            QString err = reply->errorString();
            reply->deleteLater();
            if (onFailure)
                onFailure(err);
            return;
        }

        QString result = reply->readAll();
        reply->deleteLater();
        if (onSuccess)
            onSuccess(result);
    });
}
