/* RechargeTimeView.qml  —— GPU 时长 / 包机 购买页面（v2）
 * 用法：navigateTo("qrc:/gui/RechargeTimeView.qml", "RechargeTimeView")
 * 特性：
 *   • 顶部标签居中（计时 / 包机），默认计时高亮
 *   • GPU 卡片横向排布，纵向滚动（Flow + Flickable.VerticalFlick）
 *   • 包机模式：卡片内出现「包天 / 包周 / 包月」三档切换且实时刷新价格
 *   • 全部 ReferenceError 修复
 */

import QtQuick 2.9
import QtQuick.Controls 2.2
import QtQuick.Layouts 1.3

Item {
    id: root
    anchors.fill: parent
    focus: true

    /* 0 = 计时   1 = 包机 */
    property int currentTab: 0

    /* ---------- 顶部标签（居中） ---------- */
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

    /* ---------- GPU 数据 ---------- */
    ListModel {
        id: gpuModel
        /* hourly = 按时价格   day / week / month = 包天/周/月 */
        ListElement { name: "1080显卡"; hourly: 20; day: 120; week: 700;  month: 2500 }
        ListElement { name: "2060显卡"; hourly: 20; day: 120; week: 700;  month: 2500 }
        ListElement { name: "3060显卡"; hourly: 25; day: 150; week: 900;  month: 3200 }
        ListElement { name: "4060显卡"; hourly: 30; day: 180; week: 1050; month: 3800 }
        ListElement { name: "4070显卡"; hourly: 40; day: 240; week: 1400; month: 5200 }
    }

    /* ---------- 卡片容器（纵向滚动，横向排布） ---------- */
    Flickable {
        id: flick
        anchors.top: tabRow.bottom
        anchors.topMargin: 24
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        clip: true
        contentWidth: width
        flickableDirection: Flickable.VerticalFlick

        Flow {
            id: cardFlow
            width: flick.width
            spacing: 24

            Repeater {
                model: gpuModel
                delegate: gpuCardComponent
            }
        }
    }

    /* ---------- GPU 卡片组件 ---------- */
    Component {
        id: gpuCardComponent
        Rectangle {
            id: card
            width: 240; height: currentTab === 0 ? 300 : 360
            radius: 12
            color: "#3C3C3C"
            border.color: hovered ? "#FFA500" : "#00000000"
            border.width: hovered ? 2 : 0

            /* --- 数据 --- */
            property string gpuName: name
            property int hourlyPrice: hourly
            property int dayPrice: day
            property int weekPrice: week
            property int monthPrice: month

            /* 包机选项（0/1/2） */
            property int packageMode: 0   // default 包天
            function currentPrice() {
                if (root.currentTab === 0) return hourlyPrice;
                return packageMode === 0 ? dayPrice : (packageMode === 1 ? weekPrice : monthPrice);
            }

            /* --- 视觉布局 --- */
            Column {
                anchors.fill: parent
                anchors.margins: 16
                spacing: 14
                Label {
                    text: gpuName
                    font.pixelSize: 22
                    color: "white"
                    horizontalAlignment: Text.AlignHCenter
                    width: parent.width
                }
                Label {
                    text: qsTr("最高视频码率：23 Mbps")
                    font.pixelSize: 12
                    color: "#CCCCCC"
                    horizontalAlignment: Text.AlignHCenter
                    width: parent.width
                }

                /* 包机模式下的子选项按钮 */
                Row {
                    id: pkgRow
                    visible: root.currentTab === 1
                    spacing: 4
                    anchors.horizontalCenter: parent.horizontalCenter

                    Repeater {
                        model: [qsTr("包天"), qsTr("包周"), qsTr("包月")]
                        delegate: Button {
                            id: pkgBtn
                            property int idx: index
                            checkable: true
                            checked: card.packageMode === idx
                            onClicked: card.packageMode = idx
                            implicitHeight: 24
                            implicitWidth: textItem.paintedWidth + 20
                            background: Rectangle {
                                radius: 4
                                color: pkgBtn.checked ? "#FFA500" : (pkgBtn.hovered ? "#FFA50033" : "#555555")
                            }
                            contentItem: Text {
                                id: textItem
                                text: modelData
                                font.pixelSize: 12
                                color: pkgBtn.checked ? "white" : "#DDDDDD"
                                horizontalAlignment: Text.AlignHCenter
                                verticalAlignment: Text.AlignVCenter
                            }
                        }
                    }
                }

                Row {
                    spacing: 4
                    anchors.horizontalCenter: parent.horizontalCenter
                    Label { text: qsTr("当前价格"); color: "#CCCCCC"; font.pixelSize: 14 }
                    Label {
                        text: card.currentPrice() + qsTr(" 金币") + (root.currentTab === 0 ? qsTr("/小时") : "")
                        color: "#FFA500"; font.pixelSize: 18; font.bold: true
                    }
                }

                Button {
                    id: startBtn
                    text: qsTr("开机")
                    width: 100; height: 36
                    font.pixelSize: 16
                    anchors.horizontalCenter: parent.horizontalCenter
                    background: Rectangle {
                        radius: 18
                        color: startBtn.pressed ? "#E69100" : (startBtn.hovered ? "#FFA726" : "#FF9500")
                    }
                    onClicked: {
                        console.log("Start", gpuName, card.currentPrice())
                        // TODO: 调用后端接口
                    }
                }
            }

            /* --- 交互 --- */
            property bool hovered: false
            MouseArea {
                anchors.fill: parent
                hoverEnabled: true
                onEntered: card.hovered = true
                onExited: card.hovered = false
            }
        }
    }
}
