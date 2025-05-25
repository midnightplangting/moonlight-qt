import QtQuick 2.9
import QtQuick.Controls 2.2
import QtQuick.Layouts 1.3

Item {
    id: billDetailPage

    ListModel {
        id: billModel
        ListElement { orderId: "A1001"; startTime: "2024-05-01 12:00"; endTime: "2024-05-01 14:00"; deviceId: "RTX-001"; cost: 20 }
        ListElement { orderId: "A1002"; startTime: "2024-05-02 15:30"; endTime: "2024-05-02 18:00"; deviceId: "RTX-002"; cost: 35 }
        ListElement { orderId: "A1003"; startTime: "2024-05-03 10:00"; endTime: "2024-05-03 12:00"; deviceId: "RTX-003"; cost: 25 }
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
                        onClicked: console.log("点击订单：", orderId)
                    }

                    Column {
                        anchors.fill: parent
                        anchors.margins: 16
                        spacing: 6

                        Text { text: "订单 ID：" + orderId; font.pixelSize: 16; color: "white" }
                        Text { text: "开始时间：" + startTime; font.pixelSize: 14; color: "#CCCCCC" }
                        Text { text: "关闭时间：" + endTime; font.pixelSize: 14; color: "#CCCCCC" }
                        Text { text: "机器号：" + deviceId; font.pixelSize: 14; color: "#CCCCCC" }
                        Text { text: "扣费：" + cost + " 金币"; font.pixelSize: 14; color: "#FFA500" }
                    }
                }
            }
        }
    }
}
