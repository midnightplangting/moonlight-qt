import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import QtQuick.Window 2.15
import QtQuick.Controls.Material 2.15
import UserSession 1.0

/*
  RechargeView.qml  (嵌入式页面版)
  -----------------------------------------------------------------------------
  • 作为主界面内部页面使用，根元素为 Item。
  • 所有图片暂用 "qrc:/res/update.png" 占位。
  • 已修复此前截断导致的缺失代码。
*/

Item {
    id: root
    width: parent ? parent.width : 1280
    height: parent ? parent.height : 800
    property color bgColor: "#2b2b2b"
    property bool isLoggedIn: UserSession.token !== ""

    Rectangle { anchors.fill: parent; color: bgColor }

    RowLayout {
        anchors.fill: parent
        anchors.margins: 80
        spacing: 200

        /* ================= 左侧面板 ================= */
        Column {
            id: leftPanel
            width: 400; height: parent.height
            spacing: 20

            /* ---------- 用户信息 ---------- */
            // 用户信息区域
            Item {
                id: userInfoContainer
                width: parent.width
                height: 64

                Row {
                    id: userInfoRow
                    spacing: 12
                    anchors.verticalCenter: parent.verticalCenter

                    Rectangle {
                        width: 64; height: 64; radius: 32
                        Image {
                            anchors.fill: parent
                            source: "qrc:/res/profile picture.svg"
                            fillMode: Image.PreserveAspectFit
                        }
                    }

                    Column {
                        spacing: 4
                        Label {
                            text: isLoggedIn ? UserSession.username : qsTr("未登录")
                            font.pixelSize: 20
                            color: "white"
                        }
                        Label {
                            text: isLoggedIn ? qsTr("点击查看或修改资料") : qsTr("点击登录/注册")
                            font.pixelSize: 12
                            color: "#aaaaaa"
                        }
                    }
                }

                MouseArea {
                    anchors.fill: parent
                    cursorShape: Qt.PointingHandCursor
                    onClicked: {
                        if (isLoggedIn) {
                            // 👇 推出一个资料编辑页（你自己可以创建 ModifyUserView.qml）
                            stackView.push("qrc:/gui/ModifyUserView.qml")
                        } else {
                            stackView.push("qrc:/gui/LoginRegisterView.qml")
                        }
                    }
                }
            }

            // ---------- 轮播 Banner ----------
            Rectangle {
                id: bannerFrame
                width: parent.width; height: 230; radius: 8
                color: "#555555"; clip: true

                ListModel { id: bannerModel
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
                    delegate: Image { source: model.source; width: bannerFrame.width; height: bannerFrame.height; fillMode: Image.PreserveAspectCrop }
                }

                Row {
                    spacing: 6
                    anchors.horizontalCenter: parent.horizontalCenter
                    anchors.bottom: parent.bottom; anchors.bottomMargin: 10
                    Repeater {
                        model: bannerModel.count
                        Rectangle { width: 8; height: 8; radius: 4; color: index === bannerView.currentIndex ? "#ffffff" : "#888888" }
                    }
                }

                Timer { interval: 3000; running: true; repeat: true; onTriggered: bannerView.currentIndex = (bannerView.currentIndex+1)%bannerModel.count }
            }

            // ---------- 菜单列表 ----------
            ListView {
                id: menuList
                width: parent.width; height: 200
                spacing: 8; clip: true
                model: [qsTr("账单明细"), qsTr("激活体验码"), qsTr("操作手册"), qsTr("联系客服"), qsTr("关于我们")]
                delegate: Rectangle {
                    width: parent.width; height: 32
                    color: "transparent"; border.color: "transparent"
                    MouseArea {
                        anchors.fill: parent; hoverEnabled: true
                        onPressed: parent.color = "#444444"
                        onReleased: parent.color = "transparent"
                        onClicked: console.debug("点击菜单", modelData)
                    }
                    Label { anchors.verticalCenter: parent.verticalCenter; anchors.left: parent.left; anchors.leftMargin: 8; text: modelData; color: "#dddddd"; font.pixelSize: 14 }
                }
            }
        }

        /* ================= 右侧面板 ================= */
        Column {
            id: rightPanel
            Layout.fillWidth: true
            spacing: 20

            // ---------- 金币套餐 ----------
            GridLayout {
                id: comboGrid
                columns: 3; columnSpacing: 20; rowSpacing: 20
                property int selectedCombo: 1

                ListModel { id: comboModel
                    ListElement { coins: 10;  price: 1    }
                    ListElement { coins: 100; price: 10   }
                    ListElement { coins: 500; price: 50   }
                    ListElement { coins: 1000; price: 100 }
                    ListElement { coins: 2000; price: 200 }
                    ListElement { coins: 5000; price: 500 }
                    ListElement { coins: 10000; price: 1000 }
                    ListElement { coins: 20000; price: 2000 }
                }

                Repeater {
                    model: comboModel
                    Rectangle {
                        width: 150; height: 90; radius: 6
                        color: index === comboGrid.selectedCombo ? "#ffaa33" : "#606060"
                        border.color: "#aaaaaa"; border.width: 1
                        property bool hovered: false
                        Column {
                            anchors.centerIn: parent; spacing: 4
                            Label { text: model.coins + qsTr("金币"); font.pixelSize: 18; color: hovered ? "#ffe082" : "white" }
                            Label { text: "¥ " + model.price.toFixed(2); font.pixelSize: 14; color: hovered ? "#ffe082" : "white" }
                        }
                        MouseArea {
                            anchors.fill: parent; hoverEnabled: true
                            onEntered: parent.hovered = true
                            onExited: parent.hovered = false
                            onClicked: comboGrid.selectedCombo = index
                        }
                    }
                }
            }

            Label { text: qsTr("注：金币可用于计时/包时/自助机"); color: "#aaaaaa"; font.pixelSize: 12 }

            // ---------- 支付方式 ----------
            Column {
                id: paySection
                spacing: 10
                property int selectedPay: 0   // 0=微信 1=支付宝

                Label {
                    text: qsTr("支付方式")
                    color: "white"
                    font.pixelSize: 16
                }

                Rectangle {
                    width: 240
                    height: 48
                    radius: 4
                    color: "#444444"

                    RowLayout {
                        anchors.fill: parent
                        anchors.margins: 8
                        spacing: 10

                        Image {
                            source: "qrc:/res/wx.svg"
                            fillMode: Image.PreserveAspectFit
                            Layout.preferredWidth: 24
                            Layout.preferredHeight: 24
                        }

                        Label {
                            text: qsTr("微信付款")
                            color: "white"
                            font.pixelSize: 14
                        }

                        Item { Layout.fillWidth: true }

                        Rectangle {
                            width: 16
                            height: 16
                            radius: 8
                            border.color: "#ffffff"
                            color: paySection.selectedPay === 0 ? "#33cc66" : "transparent" // ✅ 改成paySection.selectedPay
                        }
                    }

                    MouseArea {
                        anchors.fill: parent
                        onClicked: paySection.selectedPay = 0
                    }
                }

                Rectangle {
                    width: 240
                    height: 48
                    radius: 4
                    color: "#444444"

                    RowLayout {
                        anchors.fill: parent
                        anchors.margins: 8
                        spacing: 10

                        Image {
                            source: "qrc:/res/zfb.svg"
                            fillMode: Image.PreserveAspectFit
                            Layout.preferredWidth: 24
                            Layout.preferredHeight: 24
                        }

                        Label {
                            text: qsTr("支付宝付款")
                            color: "white"
                            font.pixelSize: 14
                        }

                        Item { Layout.fillWidth: true }

                        Rectangle {
                            width: 16
                            height: 16
                            radius: 8
                            border.color: "#ffffff"
                            color: paySection.selectedPay === 1 ? "#33cc66" : "transparent" // ✅ 改成paySection.selectedPay
                        }
                    }

                    MouseArea {
                        anchors.fill: parent
                        onClicked: paySection.selectedPay = 1
                    }
                }
            }


            // ---------- 立即支付按钮 ----------
            Rectangle {
                id: payButton
                width: 260; height: 60; radius: 6; color: "#ffaa33"
                anchors.horizontalCenter: parent.horizontalCenter
                Label { anchors.centerIn: parent; text: qsTr("立即支付"); font.pixelSize: 20; color: "black" }
                MouseArea {
                    anchors.fill: parent; hoverEnabled: true
                    onEntered: payButton.color = "#ffbb55"
                    onExited: payButton.color = "#ffaa33"
                    onPressed: payButton.color = "#ff9933"
                    onReleased: payButton.color = containsMouse ? "#ffbb55" : "#ffaa33"
                    onClicked: {
                        const combo = comboModel.get(comboGrid.selectedCombo);
                        console.log("Pay", combo.coins, "coins by", paySection.selectedPay === 0 ? "微信" : "支付宝");
                    }
                }
            }

            Label {
                text: qsTr("购买即同意《用户协议》和《隐私政策》");
                color: "#666666"; font.pixelSize: 10; horizontalAlignment: Text.AlignHCenter; width: parent.width
            }
        }
    }
}
