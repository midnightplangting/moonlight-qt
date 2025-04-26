/* GpuCard.qml  GPU 计费卡片
 * author: you
 */
import QtQuick 2.9
import QtQuick.Controls 2.2
import QtQuick.Layouts 1.3

Rectangle {
    id: card
    width: 220; height: 260
    radius: 10
    border.width: selected ? 2 : 0
    border.color: "#FFA500"          // 选中时的描边
    color: pressed  ? "#555555"
         : hovered  ? "#4A4A4A"
         : "#3C3C3C"

    /* ---------- 对外属性 ---------- */
    property alias cardName : nameLabel.text
    property int    price   : 0
    property bool   selected: false     // 供外部手动置选中

    signal startRequested(string gpuName, int pricePerHour)

    /* ---------- 视觉 ---------- */
    Column {
        anchors.centerIn: parent
        spacing: 10

        Label {
            id: nameLabel
            font.pixelSize: 24
            color: "white"
            width: parent.width
            horizontalAlignment: Text.AlignHCenter
        }

        Label {
            text: qsTr("最高视频码率：23 Mbps")
            font.pixelSize: 12
            color: "#CCCCCC"
            width: parent.width
            horizontalAlignment: Text.AlignHCenter
        }

        Row {
            spacing: 4
            anchors.horizontalCenter: parent.horizontalCenter
            Label { text: qsTr("当前价格"); color: "#CCCCCC"; font.pixelSize: 14 }
            Label {
                text: price + qsTr(" 金币/小时")
                color: "#FFA500"; font.pixelSize: 18; font.bold: true
            }
        }

        Button {
            id: startBtn
            text: qsTr("开机")
            width: 100; height: 36
            font.pixelSize: 16
            background: Rectangle {
                anchors.fill: parent
                radius: 18
                color: startBtn.pressed ? "#E69100"
                     : startBtn.hovered ? "#FFA726"
                     : "#FF9500"
            }

            onClicked: card.startRequested(cardName, price)
        }
    }

    /* ---------- 鼠标交互区域 ---------- */
    property bool hovered: false
    property bool pressed: false

    MouseArea {
        anchors.fill: parent
        hoverEnabled: true
        onEntered:  card.hovered = true
        onExited:   card.hovered = false
        onPressed:  card.pressed = true
        onReleased: card.pressed = false
    }
}
