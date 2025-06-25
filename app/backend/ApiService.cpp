#include "ApiService.h"
#include "OkHttpUtils.h"
#include "Logger.h"
#include <QJsonDocument>
#include <QJsonObject>

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

void ApiService::registerUser(const QString& username, const QString& password, const QString& confirmPwd,
                              std::function<void(QString)> onSuccess,
                              std::function<void(QString)> onFailure)
{
    Q_UNUSED(confirmPwd);
    OkHttpUtils::builder()
        ->url("user/register")
        ->addParam("username", username)
        ->addParam("password", password)
        ->addParam("email", username + "@default.com")
        ->addParam("phone", "00000000000")
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
