import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15

import "."   // 确保同目录下能解析 CenteredGridView.qml & GPUCard.qml

Item {
    id: root
    width: parent ? parent.width : 1280
    height: parent ? parent.height : 800

    /* 0 = 计时   1 = 包机 */
    property int currentTab: 0

    /* 顶部标签 */
    Row {
        id: tabRow
        anchors.top: parent.top
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.topMargin: 16
        spacing: 8

        Repeater {
            model: [qsTr("计时"), qsTr("包机")]
            delegate: Button {
                id: tabBtn
                property int tabIndex: index
                checkable: true
                checked: tabIndex === currentTab
                onClicked: currentTab = tabIndex

                contentItem: Text {
                    text: modelData
                    font.pixelSize: 16
                    color: tabBtn.checked ? "#FFA500" : (tabBtn.hovered ? "white" : "#CCCCCC")
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                }

                background: Rectangle {
                    radius: 6
                    color: tabBtn.checked ? "#FFFFFF22" : (tabBtn.hovered ? "#FFFFFF11" : "transparent")
                    border.color: tabBtn.checked ? "#FFA500" : "transparent"
                    border.width: 1
                }
                implicitWidth: contentItem.paintedWidth + 32
                implicitHeight: 32
            }
        }
    }

    /* 示例数据 */
    ListModel {
        id: gpuModel
        ListElement { name: "1080显卡"; hourly: 20; day: 120; week: 700;  month: 2500 }
        ListElement { name: "2060显卡"; hourly: 20; day: 120; week: 700;  month: 2500 }
        ListElement { name: "3060显卡"; hourly: 25; day: 150; week: 900;  month: 3200 }
        ListElement { name: "4060显卡"; hourly: 30; day: 180; week: 1050; month: 3800 }
        ListElement { name: "4070显卡"; hourly: 40; day: 240; week: 1400; month: 5200 }
    }

    /* 卡片容器（居中 + 纵向滚动） */
    Flickable {
        id: flick
        width: Math.min(parent.width, (240 * 4 + 24 * 3) + 48)  // 4 张卡片
        height: parent.height
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.top: tabRow.bottom
        anchors.topMargin: 24
        clip: true
        flickableDirection: Flickable.VerticalFlick
        contentWidth: width

        CenteredGridView {
            id: cardGrid
            anchors.horizontalCenter: parent.horizontalCenter
            width: flick.width
            cellWidth: 240
            columnSpacing: 24
            rowSpacing: 24
            model: gpuModel

            delegate: GPUCard {
                gpuName: name
                hourlyPrice: hourly
                dayPrice: day
                weekPrice: week
                monthPrice: month
                currentTab: root.currentTab
                externalSpacing: cardGrid.rowSpacing
            }
        }
    }
}
