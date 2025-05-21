import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import QtQuick.Window 2.15
import QtQuick.Controls.Material 2.15
import UserSession 1.0

Item {
    id: root
    width: parent ? parent.width : 1280
    height: parent ? parent.height : 800
    // property color bgColor: "black"
    property bool isLoggedIn: UserSession.token !== ""

    // 背景色
    Rectangle {
        anchors.fill: parent
        color:  window.appBackgroundColor
    }

    /* ---------- 1. 顶部用户信息区（左上角） ---------- */
    Item {
        id: userInfoHeader
        width: 400; height: 64
        anchors.top: parent.top
        anchors.left: parent.left
        anchors.topMargin: 20       // 距离顶部
        anchors.leftMargin: 20      // 距离左侧

        Row {
            spacing: 12
            anchors.verticalCenter: parent.verticalCenter

            // 头像
            Rectangle {
                width: 64; height: 64; radius: 32; color: "#444444"
                Image {
                    anchors.fill: parent
                    source: "qrc:/res/profile picture.svg"
                    fillMode: Image.PreserveAspectFit
                }
            }

            // 用户名 + 提示
            Column {
                spacing: 4
                Label {
                    text: isLoggedIn ? UserSession.username : qsTr("未登录")
                    font.pixelSize: 20; color: "white"
                }
                Label {
                    text: isLoggedIn ? qsTr("点击查看或修改资料") : qsTr("点击登录/注册")
                    font.pixelSize: 12; color: "#aaaaaa"
                }
            }
        }

        // 点击跳转
        MouseArea {
            anchors.fill: parent
            cursorShape: Qt.PointingHandCursor
            onClicked: {
                if (isLoggedIn)
                    stackView.push("qrc:/gui/ModifyUserView.qml")
                else
                    stackView.push("qrc:/gui/LoginRegisterView.qml")
            }
        }
    }

    /* ---------- 2. 主体布局：左右面板（整体居中） ---------- */
    RowLayout {
        id: mainLayout
        spacing: 200
        // 上下居中排列：从用户信息区下方开始，到底部结束
        anchors.top: userInfoHeader.bottom
        anchors.topMargin: 20
        anchors.bottom: parent.bottom
        anchors.bottomMargin: 20
        // 水平填充并留白，以保持左右居中
        anchors.left: parent.left
        anchors.leftMargin: 80
        anchors.right: parent.right
        anchors.rightMargin: 80

        /* ========== 左侧面板 ========== */
        Column {
            id: leftPanel
            width: 400
            Layout.alignment: Qt.AlignTop
            spacing: 20

            // —— 菜单 + 静态图片 区块
            RowLayout {
                width: parent.width
                height: 200
                spacing: 20

                // 菜单容器（带圆角背景）
                Rectangle {
                    id: menuContainer
                    width: 160; height: parent.height
                    radius: 8; color: "#3C3C3C"; clip: true
                    ListView {
                        id: menuList
                        anchors.fill: parent
                        anchors.margins: 8
                        spacing: 8
                        model: [
                            qsTr("账单明细"),
                            qsTr("激活体验码"),
                            qsTr("联系客服"),
                            qsTr("关于我们")
                        ]
                        delegate: Rectangle {
                            width: parent.width; height: 32
                            color: "transparent"
                            Label {
                                anchors.verticalCenter: parent.verticalCenter
                                anchors.left: parent.left; anchors.leftMargin: 8
                                text: modelData
                                color: "#dddddd"; font.pixelSize: 14
                            }
                            MouseArea {
                                anchors.fill: parent; hoverEnabled: true
                                onPressed:  parent.color = "#444444"
                                onReleased: parent.color = "transparent"
                                onClicked:  console.debug("点击菜单", modelData)
                            }
                        }
                    }
                }

                // 静态图片占位
                Rectangle {
                    id: staticImage
                    Layout.fillWidth: true
                    height: 200
                    radius: 8; color: "#555555"
                    Label {
                        anchors.centerIn: parent
                        text: qsTr("图片链接")
                        color: "#0ebb76"; font.pixelSize: 20
                    }
                }
            }

            // —— 轮播 Banner 区块
            Rectangle {
                id: bannerFrame
                width: parent.width; height: 230
                radius: 8; color: "#555555"; clip: true

                ListModel {
                    id: bannerModel
                    ListElement { source: "qrc:/res/update.svg" }
                    ListElement { source: "qrc:/res/update.svg" }
                    ListElement { source: "qrc:/res/update.svg" }
                }
                ListView {
                    id: bannerView
                    anchors.fill: parent
                    orientation: ListView.Horizontal
                    model: bannerModel
                    snapMode: ListView.SnapOneItem
                    boundsBehavior: Flickable.StopAtBounds
                    interactive: false

                    // Delegate 用 Item 包裹 Image
                    delegate: Item {
                        width: bannerFrame.width
                        height: bannerFrame.height
                        Image {
                            anchors.fill: parent
                            source: model.source
                            fillMode: Image.PreserveAspectCrop
                        }
                    }
                }

                // 圆点指示
                Row {
                    spacing: 6
                    anchors.horizontalCenter: parent.horizontalCenter
                    anchors.bottom: parent.bottom; anchors.bottomMargin: 10
                    Repeater {
                        model: bannerModel.count
                        Rectangle {
                            width: 8; height: 8; radius: 4
                            color: index === bannerView.currentIndex ? "#ffffff" : "#888888"
                        }
                    }
                }

                // 自动轮播计时器
                Timer {
                    interval: 3000; running: true; repeat: true
                    onTriggered:
                        bannerView.currentIndex = (bannerView.currentIndex + 1) % bannerModel.count
                }
            }
        }

        /* ========== 右侧面板 ========== */
        Column {
            id: rightPanel
            Layout.fillWidth: true
            spacing: 20

            // —— 金币套餐 区块
            GridLayout {
                id: comboGrid
                columns: 4
                columnSpacing: 20; rowSpacing: 20
                property int selectedCombo: 1

                ListModel {
                    id: comboModel
                    ListElement { coins: 10;   price: 1;    gift: 0    }
                    ListElement { coins: 100;  price: 10;   gift: 0    }
                    ListElement { coins: 500;  price: 50;   gift: 50   }
                    ListElement { coins: 1000; price: 100;  gift: 200  }
                    ListElement { coins: 2000; price: 200;  gift: 800  }
                    ListElement { coins: 5000; price: 500;  gift: 3000 }
                    ListElement { coins: 10000; price: 1000; gift: 8000 }
                    ListElement { coins: 20000; price: 2000; gift: 20000 }
                }

                Repeater {
                    model: comboModel
                    Rectangle {
                        width: 150; height: 90; radius: 6
                        color: index === comboGrid.selectedCombo ? "#414141" : "#303030"
                        border.color: "#aaaaaa"; border.width: 1

                        Column {
                            anchors.centerIn: parent; spacing: 4

                            // 赠送徽章
                            Rectangle {
                                visible: model.gift > 0
                                width: parent.width * 0.6; height: 16; radius: 8
                                anchors.horizontalCenter: parent.horizontalCenter
                                y: -8
                                color: "#FFD43A"
                                Label {
                                    anchors.centerIn: parent
                                    text: qsTr("赠") + model.gift + qsTr("金币")
                                    font.pixelSize: 10; color: "#333333"
                                }
                            }

                            // 主文案
                            Label {
                                text: model.coins + qsTr("金币")
                                font.pixelSize: 18; color: "white"
                            }
                            Label {
                                text: "¥ " + model.price.toFixed(2)
                                font.pixelSize: 14; color: "#cccccc"
                            }
                        }

                        MouseArea {
                            anchors.fill: parent
                            onClicked: comboGrid.selectedCombo = index
                        }
                    }
                }
            }

            // —— 说明文字
            Label {
                text: qsTr("注：金币可用于计时/包时/包机")
                color: "#aaaaaa"; font.pixelSize: 12
            }

            // —— 支付方式 & 立即支付 区块
            RowLayout {
                width: parent.width; spacing: 20

                // 支付方式
                Column {
                    id: paySection; spacing: 10
                    property int selectedPay: 0
                    Layout.alignment: Qt.AlignVCenter

                    Label {
                        text: qsTr("支付方式"); color: "white"; font.pixelSize: 16
                    }

                    Repeater {
                        model: [
                            { icon: "qrc:/res/wx.svg", text: qsTr("微信付款") },
                            { icon: "qrc:/res/zfb.svg", text: qsTr("支付宝付款") }
                        ]
                        delegate: Rectangle {
                            width: 240; height: 48; radius: 4; color: "#444444"

                            RowLayout {
                                anchors.fill: parent; anchors.margins: 8; spacing: 10
                                Image {
                                    source: modelData.icon
                                    fillMode: Image.PreserveAspectFit
                                    Layout.preferredWidth: 24; Layout.preferredHeight: 24
                                }
                                Label {
                                    text: modelData.text; color: "white"; font.pixelSize: 14
                                }
                                Item { Layout.fillWidth: true }
                                Rectangle {
                                    width: 16; height: 16; radius: 8
                                    border.color: "#ffffff"
                                    color: index === paySection.selectedPay ? "#33cc66" : "transparent"
                                }
                            }

                            MouseArea {
                                anchors.fill: parent
                                onClicked: paySection.selectedPay = index
                            }
                        }
                    }
                }

                // 立即支付按钮
                Rectangle {
                    id: payButton
                    width: 260; height: 60; radius: 6; color: "#ffaa33"
                    Layout.alignment: Qt.AlignVCenter

                    Label {
                        anchors.centerIn: parent
                        text: qsTr("立即支付")
                        font.pixelSize: 20; color: "black"
                    }

                    MouseArea {
                        anchors.fill: parent; hoverEnabled: true
                        onEntered: payButton.color = "#ffbb55"
                        onExited:  payButton.color = "#ffaa33"
                        onPressed: payButton.color = "#ff9933"
                        onReleased: payButton.color = containsMouse ? "#ffbb55" : "#ffaa33"
                        onClicked: {
                            const combo = comboModel.get(comboGrid.selectedCombo)
                            console.log("Pay", combo.coins, "coins by",
                                        paySection.selectedPay === 0 ? "微信" : "支付宝")
                        }
                    }
                }
            }

            // —— 隐私协议提示
            Label {
                text: qsTr("购买即同意《用户协议》和《隐私政策》")
                color: "#666666"; font.pixelSize: 10
                horizontalAlignment: Text.AlignHCenter; width: parent.width
            }
        }
    }
}
