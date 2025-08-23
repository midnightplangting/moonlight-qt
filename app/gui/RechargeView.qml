import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import QtQuick.Window 2.15
import QtQuick.Controls.Material 2.15
import UserSession 1.0
import UserService 1.0

Item {
    id: root
    width: parent ? parent.width : 1280
    height: parent ? parent.height : 800
    property bool isLoggedIn: UserSession.token !== ""

    StackView.onActivated: {
        userService.getGoldCoinPriceList()
    }

    UserService {
        id: userService
        onExchangeCouponSuccess: {
            couponMessageDialog.text = message
            couponMessageDialog.open()
        }
        onExchangeCouponFailure: {
            couponMessageDialog.text = errorMsg
            couponMessageDialog.open()
        }
        onGoldCoinPriceListSuccess: function(list) {
            comboModel.clear()
            for (var i = 0; i < list.length; ++i) {
                var it = list[i]
                comboModel.append({
                    id: it.goldCoinPriceId,
                    coins: it.numberOfGoldCoins,
                    price: it.price,
                    gift: it.numberOfGoldCoinsGifted ? it.numberOfGoldCoinsGifted : 0
                })
            }
            comboGrid.selectedCombo = 0
        }
        onGoldCoinPriceListFailure: function(err) {
            console.log("Failed to load price list", err)
        }
        onPayPCSuccess: function(qr) {
            payQrDialog.qrData = qr
            payQrDialog.open()
        }
        onPayPCFailure: function(err) {
            couponMessageDialog.text = err
            couponMessageDialog.open()
        }
    }

    Rectangle {
        anchors.fill: parent
        color: window.appBackgroundColor
        z: -1  // 放在最底层
    }

    ColumnLayout {
        id: pageLayout
        anchors.top: parent.top
        anchors.topMargin: 20
        anchors.horizontalCenter: parent.horizontalCenter
        spacing: 30
        width: Math.min(parent.width, 1080)

        Item {
            id: userInfoHeader
            width: 400; height: 64
            Layout.alignment: Qt.AlignLeft
            Layout.topMargin: 0
            Layout.leftMargin: 0

            Row {
                spacing: 12
                anchors.fill: parent
                anchors.margins: 0
                anchors.verticalCenter: parent.verticalCenter

                Rectangle {
                    width: 64; height: 64; radius: 32; color: "#444444"
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
                        font.pixelSize: 20; color: "#ffffff"; font.bold: true
                    }
                    Label {
                        text: isLoggedIn ? qsTr("点击查看或修改资料") : qsTr("点击登录/注册")
                        font.pixelSize: 16; color: "#ffffff"
                    }
                }
            }

            MouseArea {
                anchors.fill: parent
                cursorShape: Qt.PointingHandCursor
                onClicked: {
                    if (isLoggedIn) profileOverlay.open()
                    else            loginOverlay.open()
                }
            }

        }

        // 主体布局：左右面板
        RowLayout {
            id: mainContent
            Layout.alignment: Qt.AlignHCenter
            spacing: 80
            Layout.fillWidth: true

            // 左侧面板
            Column {
                id: leftPanel
                width: 560
                spacing: 20
                Layout.alignment: Qt.AlignTop  // 与右侧对齐同一水平基线

                // 菜单 + 静态图片
                RowLayout {
                    width: parent.width; height: 260; spacing: 16
                    Rectangle {
                        width: 140; height: parent.height; radius: 12; color: "#1C1C1E"; clip: true
                        ListView {
                            anchors.fill: parent; anchors.margins: 8; spacing: 8
                            model: [qsTr("账单明细"), qsTr("激活兑换码"), qsTr("联系客服"), qsTr("关于我们")]
                            delegate: Rectangle {
                                width: parent.width; height: 32; color: "transparent"
                                Label {
                                    anchors.verticalCenter: parent.verticalCenter
                                    anchors.left: parent.left; anchors.leftMargin: 8
                                    text: modelData; color: "#ffffff"; font.pixelSize: 16;font.bold: true
                                }
                                MouseArea { anchors.fill: parent; hoverEnabled: true
                                    onPressed: parent.color = "#1C1C1E"
                                    onReleased: parent.color = "transparent"
                                    onClicked: {
                                        console.debug("点击菜单", modelData)
                                        if (modelData === qsTr("账单明细")) {
                                            stackView.push("qrc:/gui/BillDetailView.qml")
                                        } else if (modelData === qsTr("激活兑换码")) {
                                            couponInputDialog.open()
                                        } else if (modelData === qsTr("联系客服")) {
                                            stackView.push("qrc:/gui/ContactCustomerView.qml")
                                        } else if (modelData === qsTr("关于我们")) {
                                            stackView.push("qrc:/gui/AboutUsView.qml")
                                        }
                                    }
                                }
                            }
                        }
                    }
                    Rectangle {
                        Layout.fillWidth: true; height: 260; radius: 12; color: "#1C1C1E"
                        Label {
                            anchors.centerIn: parent
                            text: qsTr("敬请期待..."); color: "#0ebb76"; font.pixelSize: 20
                        }
                    }
                }
                Rectangle {
                    width: parent.width; height: 280; radius: 12; color: "#1C1C1E"
                    Label {
                        anchors.centerIn: parent
                        text: qsTr("敬请期待..."); color: "#0ebb76"; font.pixelSize: 20
                    }
                }
                // 轮播Banner
                // Rectangle {
                //     id: bannerFrame
                //     width: parent.width; height: 280; radius: 12; color: "#1C1C1E"; clip: true
                //     ListModel { id: bannerModel
                //         ListElement { source: "qrc:/res/logo.svg" }
                //         ListElement { source: "qrc:/res/logo.svg" }
                //         ListElement { source: "qrc:/res/logo.svg" }
                //     }
                //     ListView {
                //         id: bannerView
                //         anchors.fill: parent; orientation: ListView.Horizontal
                //         model: bannerModel; snapMode: ListView.SnapOneItem; boundsBehavior: Flickable.StopAtBounds; interactive: false
                //         delegate: Item { width: bannerFrame.width; height: bannerFrame.height
                //             Image { anchors.fill: parent; source: model.source; fillMode: Image.PreserveAspectCrop }
                //         }
                //     }
                //     Row { spacing: 6; anchors.horizontalCenter: parent.horizontalCenter; anchors.bottom: parent.bottom; anchors.bottomMargin: 6
                //         Repeater { model: bannerModel.count
                //             Rectangle { width: 8; height: 8; radius: 12; color: index === bannerView.currentIndex ? "#ffffff" : "#1C1C1E" }
                //         }
                //     }
                //     Timer { interval: 3000; running: true; repeat: true; onTriggered: bannerView.currentIndex = (bannerView.currentIndex + 1) % bannerModel.count }
                // }
            }

            // ===== 右侧面板 =====
            Column {
                id: rightPanel
                spacing: 20
                Layout.fillWidth: true
                Layout.alignment: Qt.AlignTop  // 与左侧同一水平基线

                // 套餐格
                GridLayout {
                    id: comboGrid; columns: 3; columnSpacing: 12; rowSpacing: 12; property int selectedCombo: 0
                    ListModel { id: comboModel }
                    Repeater { model: comboModel
                        Rectangle {
                            id: comboCard
                            width: 150; height: 90; radius: 12;
                            color: index === comboGrid.selectedCombo ? "#3A3A3C" : "#1C1C1E"; clip: true
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
                                Label { anchors.centerIn: parent; anchors.verticalCenterOffset: 2; text: qsTr("赠") + model.gift + qsTr("金币"); font.pixelSize: 14; font.bold: true; color: "#333333" }
                            }
                            // 主信息
                            ColumnLayout { anchors.centerIn: parent; spacing: 4; width: parent.width
                                Label { text: model.coins + qsTr("金币"); font.pixelSize: 18; font.bold: true; color: "#FFC000"; Layout.alignment: Qt.AlignHCenter; Layout.topMargin: 15 }
                                Label { text: "¥ " + model.price.toFixed(2); font.pixelSize: 14; font.bold: true; color: "#cccccc"; Layout.alignment: Qt.AlignHCenter }
                            }
                        }
                    }
                }
                Label { text: qsTr("注：金币可用于计时/包时/包机"); color:"#aaaaaa"; font.pixelSize:15 }
                // 支付方式 & 按钮
                RowLayout { spacing:20
                    Column { id:paySection; spacing:10; property int selectedPay:0; Layout.alignment:Qt.AlignVCenter
                        Label { text: qsTr("支付方式"); color:"white"; font.pixelSize:16; font.bold: true}
                        Repeater {
                            model:[{icon:"qrc:/res/wx.svg",text:qsTr("微信付款")},{icon:"qrc:/res/zfb.svg",text:qsTr("支付宝付款")}]
                            delegate: Rectangle { width:200;height:48;radius:12;color:"#444444"
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
                        id: payButton; width: 240; height: 60; radius: 40; color: "#FFBF00"; Layout.topMargin: 30
                        scale: 1; transformOrigin: Item.Center
                        Behavior on scale { NumberAnimation { duration: 100; easing.type: Easing.OutQuad } }
                        Label { anchors.centerIn: parent; text: qsTr("立即支付"); font.pixelSize: 25; color: "black"; font.bold: true }
                        MouseArea {
                            anchors.fill: parent; hoverEnabled: true
                            onEntered: payButton.scale = 1.05
                            onExited: payButton.scale = 1.0
                            onPressed: payButton.scale = 0.95
                            onReleased: payButton.scale = 1.05
                            onClicked: {
                                const combo = comboModel.get(comboGrid.selectedCombo)
                                console.log("Pay", combo.coins, "coins by", paySection.selectedPay === 0 ? "微信" : "支付宝")
                                if (paySection.selectedPay === 0) {
                                    userService.payWxPC(combo.id)
                                } else {
                                    userService.payAliPC(combo.id)
                                }
                            }
                        }
                    }
                }
                Text {
                    textFormat: Text.RichText
                    text: qsTr("<span style=\"color:white; \">购买即同意<a href=\"UserAgreement\" style=\"color:white; font-weight:bold;\">《用户协议》</a>和<a href=\"PrivacyPolicy\" style=\"color:white; font-weight:bold;\">《隐私政策》</a></span>")
                    color: "#ffffff"
                    font.pixelSize: 15
                    horizontalAlignment: Text.AlignHCenter
                    width: parent.width
                    onLinkActivated: function(link) {
                        if (link === "UserAgreement") {
                            stackView.push("qrc:/gui/UserAgreementView.qml")
                        } else if (link === "PrivacyPolicy") {
                            stackView.push("qrc:/gui/PrivacyPolicyView.qml")
                        }
                    }
                }
            }
        }
    }
    // 遮罩背景 + 弹窗浮层
    Popup {
        id: profileOverlay
        modal: true
        dim: true
        focus: true
        // Overlay.overlay isn't available on older Qt versions
        parent: ApplicationWindow.contentItem
        x: Math.round((parent.width - width) / 2)
        y: Math.round((parent.height - height) / 2)
        closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutside


        contentItem: ModifyUserView {
            onRequestClose: profileOverlay.close()
        }
    }

    Popup {
        id: loginOverlay
        modal: true
        dim: true
        focus: true
        // Overlay.overlay isn't available on older Qt versions
        parent: ApplicationWindow.contentItem
        x: Math.round((parent.width - width) / 2)
        y: Math.round((parent.height - height) / 2)
        closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutside

        contentItem: LoginRegisterView {
            onRequestClose: loginOverlay.close()
            onLoginSucceeded: {
                // 重新载入用户中心界面以刷新数据
                stackView.replace(stackView.currentItem, "qrc:/gui/RechargeView.qml")
            }
        }
    }

    NavigableDialog {
        id: couponInputDialog
        standardButtons: Dialog.Ok
        closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutside
        onAccepted: {
            if (couponField.text.length > 0)
                userService.exchangeCoupon(couponField.text)
        }

        ColumnLayout {
            anchors.fill: parent
            spacing: 10
            Label {
                text: qsTr("请输入兑换码")
                Layout.alignment: Qt.AlignHCenter
            }
            TextField {
                id: couponField
                Layout.fillWidth: true
                placeholderText: qsTr("兑换码")
            }
        }
    }

    NavigableMessageDialog {
        id: couponMessageDialog
        standardButtons: Dialog.Ok
    }

    Popup {
        id: payQrDialog
        modal: true
        dim: true
        focus: true
        parent: ApplicationWindow.contentItem
        x: Math.round((parent.width - width) / 2)
        y: Math.round((parent.height - height) / 2)
        property string qrData: ""

        ColumnLayout {
            anchors.centerIn: parent
            spacing: 20

            Image {
                source: payQrDialog.qrData
                width: 200
                height: 200
                fillMode: Image.PreserveAspectFit
            }

            Rectangle {
                width: 180
                height: 40
                radius: 20
                color: "#FFBF00"
                Layout.alignment: Qt.AlignHCenter  // 加这一行来居中对齐
                Text {
                    anchors.centerIn: parent
                    text: qsTr("支付成功后点击此处")
                    color: "black"
                    font.bold: true
                }
                MouseArea {
                    anchors.fill: parent
                    onClicked: payQrDialog.close()
                }
            }
        }

    }
}
