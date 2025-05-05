import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15

/*
    LoginRegisterView.qml
    --------------------
    登录 / 注册界面（中文注释 + 简洁排版）
    • 根元素 Item，可嵌入 StackView
    • 卡片居中，半透明背景，白色文字
    • 仅使用 QtQuick 基础模块，Qt 6.9 默认即可运行
*/

Item {
    id: root
    anchors.fill: parent

    // ------------------ 对外属性 / 信号 ------------------
    property alias 用户名: loginUserField.text
    property alias 密码:   loginPassField.text
    property alias 确认密码: regConfirmField.text
    signal 请求登录(string user, string pwd)
    signal 请求注册(string user, string pwd, string confirmPwd)

    // ------------------ 颜色配置 ------------------
    readonly property color cardColor: "#00000055"
    readonly property color borderColor: "white"

    /* 阴影：用一个稍大的半透明矩形实现，无需额外模块 */
    Rectangle {
        anchors.centerIn: card
        width: card.width; height: card.height
        color: "#00000054"
        radius: 18
        anchors.verticalCenterOffset: 6
        z: -1
    }

    /* 主卡片 */
    Rectangle {
        id: card
        anchors.centerIn: parent
        width: Math.min(parent.width * 0.85, 460)
        height: Math.min(parent.height * 0.85, 460)
        radius: 18
        color: cardColor
        border.color: "#00000055"; border.width: 1

        Column {
            anchors.fill: parent
            anchors.margins: 32
            spacing: 24



            /* 登录 / 注册 标签 */
            TabBar {
                id: tabBar
                anchors.horizontalCenter: parent.horizontalCenter
                contentWidth: card.width * 0.5
                background: Rectangle { color: "transparent" }

                TabButton { text: qsTr("登录") }
                TabButton { text: qsTr("注册") }
            }

            /* 表单 StackLayout */
            StackLayout {
                id: stack
                currentIndex: tabBar.currentIndex
                width: parent.width

                /* -------- 登录表单 -------- */
                Column {
                    spacing: 16
                    width: parent.width

                    // 用户名
                    TextField {
                        id: loginUserField
                        placeholderText: qsTr("用户名 / 邮箱")
                        font.pixelSize: 16
                        color: "white"
                        placeholderTextColor: borderColor
                        width: parent.width
                        height: 40
                        background: Rectangle {
                            radius: 8
                            color: "#00000055"
                            border.color: borderColor; border.width: 1
                        }
                    }

                    // 密码
                    TextField {
                        id: loginPassField
                        placeholderText: qsTr("密码")
                        echoMode: TextInput.Password
                        font.pixelSize: 16
                        color: "white"
                        placeholderTextColor: borderColor
                        width: parent.width
                        height: 40
                        background: Rectangle {
                            radius: 8
                            color: "#00000055"
                            border.color: borderColor; border.width: 1
                        }
                    }

                    // 登录按钮
                    Button {
                        text: qsTr("登录")
                        width: parent.width
                        height: 42
                        onClicked: 请求登录(loginUserField.text, loginPassField.text)
                        background: Rectangle {
                            radius: 8
                            color: "transparent"
                            border.color: "white"; border.width: 1
                        }
                        contentItem: Label { text: parent.text; color: "white"; anchors.centerIn: parent }
                    }
                }

                /* -------- 注册表单 -------- */
                Column {
                    spacing: 16
                    width: parent.width

                    // 用户名
                    TextField {
                        id: regUserField
                        placeholderText: qsTr("用户名")
                        font.pixelSize: 16
                        color: "white"
                        placeholderTextColor: borderColor
                        width: parent.width
                        height: 40
                        background: Rectangle {
                            radius: 8
                            color: "#00000055"
                            border.color: borderColor; border.width: 1
                        }
                    }

                    // 密码
                    TextField {
                        id: regPassField
                        placeholderText: qsTr("密码")
                        echoMode: TextInput.Password
                        font.pixelSize: 16
                        color: "white"
                        placeholderTextColor: borderColor
                        width: parent.width
                        height: 40
                        background: Rectangle {
                            radius: 8
                            color: "#00000055"
                            border.color: borderColor; border.width: 1
                        }
                    }

                    // 确认密码
                    TextField {
                        id: regConfirmField
                        placeholderText: qsTr("确认密码")
                        echoMode: TextInput.Password
                        font.pixelSize: 16
                        color: "white"
                        placeholderTextColor: borderColor
                        width: parent.width
                        height: 40
                        background: Rectangle {
                            radius: 8
                            color: "#00000055"
                            border.color: borderColor; border.width: 1
                        }
                    }

                    // 注册按钮
                    Button {
                        text: qsTr("注册")
                        width: parent.width
                        height: 42
                        onClicked: 请求注册(regUserField.text, regPassField.text, regConfirmField.text)
                        background: Rectangle {
                            radius: 8
                            color: "transparent"
                            border.color: "white"; border.width: 1
                        }
                        contentItem: Label { text: parent.text; color: "white"; anchors.centerIn: parent }
                    }
                }
            }
        }
    }
}
