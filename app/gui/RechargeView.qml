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
    property bool isLoggedIn: UserSession.token !== ""

    // 背景色
    Rectangle {
        anchors.fill: parent
        color: window.appBackgroundColor
    }

    // ---------- 页面容器：居中并限宽 ----------
    ColumnLayout {
        id: pageLayout
        anchors.top: parent.top               // 固定在顶部
        anchors.topMargin: 20                 // 与顶部距离
        anchors.horizontalCenter: parent.horizontalCenter  // 水平居中
        spacing: 30
        width: Math.min(parent.width, 1080)   // 最大宽度限制

        // ---------- 1. 用户信息区：水平居中后左移 ----------
        Item {
            id: userInfoHeader
            width: 400; height: 64
            Layout.alignment: Qt.AlignLeft
            Layout.topMargin: 0      // 上边距由页面容器的 anchors.topMargin 控制
            Layout.leftMargin: 0     // 左边距可以调整

            Row {
                spacing: 12
                anchors.fill: parent
                anchors.margins: 0
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

                // 用户名及提示
                Column {
                    spacing: 4
                    Label {
                        text: isLoggedIn ? UserSession.username : qsTr("未登录")
                        font.pixelSize: 20; color: "#ffffff"; font.bold: true
                    }
                    Label {
                        text: isLoggedIn ? qsTr("点击查看或修改资料") : qsTr("点击登录/注册")
                        font.pixelSize: 12; color: "#ffffff"
                    }
                }
            }

            MouseArea {
                anchors.fill: parent
                cursorShape: Qt.PointingHandCursor
                onClicked: {
                    if (isLoggedIn) stackView.push("qrc:/gui/ModifyUserView.qml")
                    else            stackView.push("qrc:/gui/LoginRegisterView.qml")
                }
            }
        }

        // ---------- 2. 主体布局：左右面板 ----------
        RowLayout {
            id: mainContent
            Layout.alignment: Qt.AlignHCenter
            spacing: 80
            Layout.fillWidth: true

            // ===== 左侧面板 =====
            Column {
                id: leftPanel
                width: 560
                spacing: 20
                Layout.alignment: Qt.AlignTop  // 与右侧对齐同一水平基线

                // 菜单 + 静态图片
                RowLayout {
                    width: parent.width; height: 260; spacing: 16
                    Rectangle {
                        width: 140; height: parent.height; radius: 12; color: "#1C1C14"; clip: true
                        ListView {
                            anchors.fill: parent; anchors.margins: 8; spacing: 8
                            model: [qsTr("账单明细"), qsTr("激活体验码"), qsTr("联系客服"), qsTr("关于我们")]
                            delegate: Rectangle {
                                width: parent.width; height: 32; color: "transparent"
                                Label {
                                    anchors.verticalCenter: parent.verticalCenter
                                    anchors.left: parent.left; anchors.leftMargin: 8
                                    text: modelData; color: "#ffffff"; font.pixelSize: 16;font.bold: true
                                }
                                MouseArea { anchors.fill: parent; hoverEnabled: true
                                    onPressed: parent.color = "#1C1C14"
                                    onReleased: parent.color = "transparent"
                                    onClicked: console.debug("点击菜单", modelData)
                                }
                            }
                        }
                    }
                    Rectangle {
                        Layout.fillWidth: true; height: 260; radius: 12; color: "#1C1C14"
                        Label {
                            anchors.centerIn: parent
                            text: qsTr("图片链接"); color: "#0ebb76"; font.pixelSize: 20
                        }
                    }
                }

                // 轮播Banner
                Rectangle {
                    id: bannerFrame
                    width: parent.width; height: 280; radius: 12; color: "#1C1C14"; clip: true
                    ListModel { id: bannerModel
                        ListElement { source: "qrc:/res/update.svg" }
                        ListElement { source: "qrc:/res/update.svg" }
                        ListElement { source: "qrc:/res/update.svg" }
                    }
                    ListView {
                        id: bannerView
                        anchors.fill: parent; orientation: ListView.Horizontal
                        model: bannerModel; snapMode: ListView.SnapOneItem; boundsBehavior: Flickable.StopAtBounds; interactive: false
                        delegate: Item { width: bannerFrame.width; height: bannerFrame.height
                            Image { anchors.fill: parent; source: model.source; fillMode: Image.PreserveAspectCrop }
                        }
                    }
                    Row { spacing: 6; anchors.horizontalCenter: parent.horizontalCenter; anchors.bottom: parent.bottom; anchors.bottomMargin: 6
                        Repeater { model: bannerModel.count
                            Rectangle { width: 8; height: 8; radius: 12; color: index === bannerView.currentIndex ? "#ffffff" : "#1C1C14" }
                        }
                    }
                    Timer { interval: 3000; running: true; repeat: true; onTriggered: bannerView.currentIndex = (bannerView.currentIndex + 1) % bannerModel.count }
                }
            }

            // ===== 右侧面板 =====
            Column {
                id: rightPanel
                spacing: 20
                Layout.fillWidth: true
                Layout.alignment: Qt.AlignTop  // 与左侧同一水平基线

                // 套餐格
                GridLayout {
                    id: comboGrid; columns: 3; columnSpacing: 12; rowSpacing: 12; property int selectedCombo: 1
                    ListModel { id: comboModel
                        ListElement { coins: 10; price: 1; gift: 0 }
                        ListElement { coins: 100; price: 10; gift: 0 }
                        ListElement { coins: 500; price: 50; gift: 50 }
                        ListElement { coins: 1000; price: 100; gift: 200 }
                        ListElement { coins: 2000; price: 200; gift: 800 }
                        ListElement { coins: 5000; price: 500; gift: 3000 }
                        ListElement { coins: 10000; price: 1000; gift: 8000 }
                        ListElement { coins: 20000; price: 2000; gift: 20000 }
                    }
                    Repeater { model: comboModel
                        Rectangle {
                            id: comboCard
                            width: 150; height: 90; radius: 12;
                            color: index === comboGrid.selectedCombo ? "#3A3A3C" : "#1C1C14"; clip: true
                            scale: 1; transformOrigin: Item.Center
                            Behavior on scale { NumberAnimation { duration: 100; easing.type: Easing.OutQuad } }

                            MouseArea {
                                anchors.fill: parent; hoverEnabled: true
                                onEntered: comboCard.scale = 1.05
                                onExited: comboCard.scale = 1.0
                                onPressed: comboCard.scale = 0.95
                                onReleased: comboCard.scale = 1.05
                                onClicked: comboGrid.selectedCombo = index
                            }

                            // 赠送徽章
                            Rectangle { visible: model.gift > 0; width: parent.width * 0.7; height: 20; radius: 6; anchors.horizontalCenter: parent.horizontalCenter; y: -3; color: "#FFBF00"
                                Label { anchors.centerIn: parent; text: qsTr("赠") + model.gift + qsTr("金币"); font.pixelSize: 14; font.bold: true; color: "#333333" }
                            }
                            // 主信息
                            Column { anchors.centerIn: parent; spacing: 4
                                Label { text: model.coins + qsTr("金币"); font.pixelSize: 18; font.bold: true; color: "#FFC000" }
                                Label { text: "¥ " + model.price.toFixed(2); font.pixelSize: 14; font.bold: true; color: "#cccccc" }
                            }
                        }
                    }
                }
                Label { text: qsTr("注：金币可用于计时/包时/包机"); color:"#aaaaaa"; font.pixelSize:15 }
                // 支付方式 & 按钮
                RowLayout { spacing:20
                    Column { id:paySection; spacing:10; property int selectedPay:0; Layout.alignment:Qt.AlignVCenter
                        Label { text: qsTr("支付方式"); color:"white"; font.pixelSize:16 }
                        Repeater { model:[{icon:"qrc:/res/wx.svg",text:qsTr("微信付款")},{icon:"qrc:/res/zfb.svg",text:qsTr("支付宝付款")}]
                            delegate: Rectangle { width:200;height:48;radius:4;color:"#444444"
                                RowLayout { anchors.fill:parent;anchors.margins:8;spacing:10
                                    Image { source:modelData.icon; fillMode:Image.PreserveAspectFit;Layout.preferredWidth:24;Layout.preferredHeight:24 }
                                    Label { text:modelData.text;color:"#ffffff";font.pixelSize:14 }
                                    Item{Layout.fillWidth:true}
                                    Rectangle{width:16;height:16;radius:8;border.color:"#ffffff";color:index===paySection.selectedPay?"#33cc66":"transparent"}
                                }
                                MouseArea{anchors.fill:parent;onClicked:paySection.selectedPay=index}
                            }
                        }
                    }
                    // 立即支付按钮
                    Rectangle {
                        id: payButton; width: 240; height: 60; radius: 6; color: "#FFBF00"
                        scale: 1; transformOrigin: Item.Center
                        Behavior on scale { NumberAnimation { duration: 100; easing.type: Easing.OutQuad } }
                        Label { anchors.centerIn: parent; text: qsTr("立即支付"); font.pixelSize: 25; color: "black"; font.bold: true }
                        MouseArea {
                            anchors.fill: parent; hoverEnabled: true
                            onEntered: payButton.scale = 1.05
                            onExited: payButton.scale = 1.0
                            onPressed: payButton.scale = 0.95
                            onReleased: payButton.scale = 1.05
                            onClicked: { const combo = comboModel.get(comboGrid.selectedCombo); console.log("Pay", combo.coins, "coins by", paySection.selectedPay === 0 ? "微信" : "支付宝") }
                        }
                    }
                }
                Label { text:qsTr("购买即同意《用户协议》和《隐私政策》"); color:"#ffffff";font.pixelSize:15;horizontalAlignment:Text.AlignHCenter;width:parent.width }
            }
        }
    }
}
