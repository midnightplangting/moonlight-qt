import QtQuick 2.9
import QtQuick.Controls 2.2
import QtQuick.Layouts 1.3
import ComputerManager 1.0

Item {
    id: billDetailPage

    Component.onCompleted: {
        ComputerManager.getOrderDetailList()
    }

    Connections {
        target: ComputerManager
        onGetOrderDetailListFinished: function(success, data) {
            if (!success) {
                console.warn("getOrderDetailList failed: " + data)
                return
            }
            var obj = JSON.parse(data)
            if (obj.code === 200 && obj.data && obj.data.ordersList) {
                billModel.clear()
                for (var i = 0; i < obj.data.ordersList.length; i++) {
                    var o = obj.data.ordersList[i]
                    billModel.append({
                        deviceName: o.deviceName,
                        billingType: o.billingType,
                        coins: o.coins,
                        serviceStartTime: o.serviceStartTime,
                        serviceEndTime: o.serviceEndTime,
                        status: o.status
                    })
                }
            }
        }
    }

    ListModel {
        id: billModel
    }

    Flickable {
        id: flick
        anchors.fill: parent
        contentHeight: listContent.height
        flickableDirection: Flickable.VerticalFlick

        Column {
            id: listContent
            width: flick.width
            spacing: 16
            anchors.top: parent.top
            anchors.horizontalCenter: parent.horizontalCenter
            anchors.topMargin: 20

            Repeater {
                model: billModel

                delegate: Rectangle {
                    id: card
                    width: parent.width - 30
                    height: 150
                    radius: 10
                    anchors.horizontalCenter: parent.horizontalCenter
                    color: hovered ? "#2A2A2A" : "#1C1C1E"
                    scale: 1

                    Behavior on scale { NumberAnimation { duration: 100; easing.type: Easing.OutQuad } }

                    property bool hovered: false

                    MouseArea {
                        anchors.fill: parent
                        hoverEnabled: true
                        onEntered: { hovered = true; card.scale = 1.02 }
                        onExited:  { hovered = false; card.scale = 1.0 }
                        onPressed: card.scale = 0.95
                        onReleased: card.scale = 1.02
                        onClicked: console.log("点击订单：", deviceName)
                    }

                    Column {
                        anchors.fill: parent
                        anchors.margins: 16
                        spacing: 6

                        Text { text: "机器号：" + deviceName; font.pixelSize: 16; color: "white" }
                        Text { text: "开始时间：" + serviceStartTime; font.pixelSize: 14; color: "#CCCCCC" }
                        Text { text: "结束时间：" + serviceEndTime; font.pixelSize: 14; color: "#CCCCCC" }
                        Text { text: "扣费：" + coins + " 金币"; font.pixelSize: 14; color: "#FFA500" }
                    }
                }
            }
        }
    }
}
