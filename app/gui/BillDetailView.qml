import QtQuick 2.9
import QtQuick.Controls 2.2
import QtQuick.Layouts 1.3
import ComputerManager 1.0

Item {
    id: billDetailPage

    // 将时间字符串转换为本地时间格式
    function formatDateTime(str) {
        if (!str) return "--"
        var dt = new Date(str)
        if (isNaN(dt)) return str
        return Qt.formatDateTime(dt, "yyyy-MM-dd hh:mm:ss")
    }

    // 计算使用时长(分钟)
    function calcDuration(start, end) {
        if (!start) return "--"
        var st = new Date(start)
        var et = end ? new Date(end) : new Date()
        if (isNaN(st) || isNaN(et)) return "--"
        return Math.floor((et - st) / 60000)
    }

    // 根据计费类型返回文字
    function billingTypeText(bt) {
        switch(bt) {
        case 1: return qsTr("计时")
        case 2: return qsTr("包天")
        case 3: return qsTr("包周")
        case 4: return qsTr("包月")
        case 7: return qsTr("包时")
        default: return ""
        }
    }

    // 时间进制转换，复用PcView中的实现
    function formatDuration(minutes) {
        minutes = parseInt(minutes)
        if (isNaN(minutes) || minutes < 0)
            return "--"

        const MONTH_MINUTES = 30 * 24 * 60
        const WEEK_MINUTES = 7 * 24 * 60
        const DAY_MINUTES = 24 * 60
        const HOUR_MINUTES = 60

        const months = Math.floor(minutes / MONTH_MINUTES)
        minutes %= MONTH_MINUTES

        const weeks = Math.floor(minutes / WEEK_MINUTES)
        minutes %= WEEK_MINUTES

        const days = Math.floor(minutes / DAY_MINUTES)
        minutes %= DAY_MINUTES

        const hours = Math.floor(minutes / HOUR_MINUTES)
        minutes %= HOUR_MINUTES

        var result = ""
        if (months > 0) result += months + qsTr("月")
        if (weeks > 0) result += weeks + qsTr("周")
        if (days > 0) result += days + qsTr("天")
        if (hours > 0) result += hours + qsTr("小时")
        if (minutes > 0 || result === "") result += minutes + qsTr("分钟")

        return result
    }

    Component.onCompleted: {
        ComputerManager.getOrderDetailList()
    }

    Connections {
        target: ComputerManager
        function onGetOrderDetailListFinished(success, data) {
            if (!success) {
                console.warn("getOrderDetailList failed: " + data)
                return
            }
            var obj = JSON.parse(data)
            if (obj.code === 200 && obj.data && obj.data.ordersList) {
                billModel.clear()
                var list = obj.data.ordersList
                for (var i = list.length - 1; i >= 0; i--) { // 倒序展示
                    var o = list[i]
                    billModel.append({
                        deviceName: o.deviceName,
                        billingType: o.billingType,
                        billingText: billingTypeText(o.billingType),
                        coins: o.coins,
                        serviceStartTime: formatDateTime(o.serviceStartTime),
                        serviceEndTime: formatDateTime(o.serviceEndTime),
                        usage: calcDuration(o.serviceStartTime, o.serviceEndTime),
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
                    height: 180
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

                        Row {
                            spacing: 8
                            Rectangle {
                                color: "#FFA54F"
                                radius: 4
                                height: 20
                                width: billingLabel.paintedWidth + 16
                                Text {
                                    id: billingLabel
                                    anchors.centerIn: parent
                                    color: "white"
                                    font.pixelSize: 13
                                    font.bold: true
                                    text: billingText
                                }
                            }
                            Text { text: qsTr("机器号：") + deviceName; font.pixelSize: 16; color: "white" }
                        }

                        Text { text: qsTr("开始时间：") + serviceStartTime; font.pixelSize: 14; color: "#CCCCCC" }
                        Text { text: qsTr("结束时间：") + serviceEndTime; font.pixelSize: 14; color: "#CCCCCC" }
                        Text { text: qsTr("使用时长：") + formatDuration(usage); font.pixelSize: 14; color: "#CCCCCC" }
                        Text { text: qsTr("状态：") + (status === 0 && !serviceEndTime ? qsTr("进行中") : qsTr("已结束")); font.pixelSize: 14; color: "#CCCCCC" }
                        Text { text: qsTr("扣费：") + coins + qsTr(" 金币"); font.pixelSize: 14; color: "#FFA500" }
                    }
                }
            }
        }
    }
}
