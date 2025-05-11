import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import UserService 1.0  // 注册的 UserService 类型

Item {
    id: root
    anchors.fill: parent

    // ------------------ 外部属性 / 信号 ------------------
    property alias username: loginUsernameField.text
    property alias password: loginPasswordField.text
    property alias confirmPassword: registerConfirmField.text
    signal requestLogin(string username, string password)
    signal requestRegister(string username, string password, string confirmPassword)

    // ------------------ 背景颜色配置 ------------------
    readonly property color cardColor: "#00000055"
    readonly property color borderColor: "white"

    // 添加 UserService 实例
    UserService {
        id: userService

        onLoginSuccess: console.log("Login success")
        onLoginFailure: console.log("Login failed:", errorMsg)

        onRegisterSuccess: console.log("Register success")
        onRegisterFailure: console.log("Register failed:", errorMsg)
    }

    // 阴影
    Rectangle {
        anchors.centerIn: card
        width: card.width; height: card.height
        color: "#00000054"
        radius: 18
        anchors.verticalCenterOffset: 6
        z: -1
    }

    // 主卡片
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

            // 登录 / 注册 标签切换
            TabBar {
                id: tabBar
                anchors.horizontalCenter: parent.horizontalCenter
                contentWidth: card.width * 0.5
                background: Rectangle { color: "transparent" }

                TabButton { text: qsTr("登录") }
                TabButton { text: qsTr("注册") }
            }

            StackLayout {
                id: stack
                currentIndex: tabBar.currentIndex
                width: parent.width

                // ---------- 登录表单 ----------
                Column {
                    spacing: 16
                    width: parent.width

                    TextField {
                        id: loginUsernameField
                        placeholderText: qsTr("用户名 / 邮箱")
                        font.pixelSize: 16
                        color: "white"
                        placeholderTextColor: borderColor
                        width: parent.width; height: 40
                        background: Rectangle {
                            radius: 8
                            color: "#00000055"
                            border.color: borderColor; border.width: 1
                        }
                    }

                    TextField {
                        id: loginPasswordField
                        placeholderText: qsTr("密码")
                        echoMode: TextInput.Password
                        font.pixelSize: 16
                        color: "white"
                        placeholderTextColor: borderColor
                        width: parent.width; height: 40
                        background: Rectangle {
                            radius: 8
                            color: "#00000055"
                            border.color: borderColor; border.width: 1
                        }
                    }

                    Button {
                        text: qsTr("登录")
                        width: parent.width; height: 42
                        onClicked: userService.login(loginUsernameField.text, loginPasswordField.text)
                        background: Rectangle {
                            radius: 8
                            color: "transparent"
                            border.color: "white"; border.width: 1
                        }
                        contentItem: Label {
                            text: parent.text; color: "white"; anchors.centerIn: parent
                        }
                    }
                }

                // ---------- 注册表单 ----------
                Column {
                    spacing: 16
                    width: parent.width

                    TextField {
                        id: registerUsernameField
                        placeholderText: qsTr("用户名")
                        font.pixelSize: 16
                        color: "white"
                        placeholderTextColor: borderColor
                        width: parent.width; height: 40
                        background: Rectangle {
                            radius: 8
                            color: "#00000055"
                            border.color: borderColor; border.width: 1
                        }
                    }

                    TextField {
                        id: registerPasswordField
                        placeholderText: qsTr("密码")
                        echoMode: TextInput.Password
                        font.pixelSize: 16
                        color: "white"
                        placeholderTextColor: borderColor
                        width: parent.width; height: 40
                        background: Rectangle {
                            radius: 8
                            color: "#00000055"
                            border.color: borderColor; border.width: 1
                        }
                    }

                    TextField {
                        id: registerConfirmField
                        placeholderText: qsTr("确认密码")
                        echoMode: TextInput.Password
                        font.pixelSize: 16
                        color: "white"
                        placeholderTextColor: borderColor
                        width: parent.width; height: 40
                        background: Rectangle {
                            radius: 8
                            color: "#00000055"
                            border.color: borderColor; border.width: 1
                        }
                    }

                    Button {
                        text: qsTr("注册")
                        width: parent.width; height: 42
                        onClicked: userService.registerUser(registerUsernameField.text, registerPasswordField.text, registerConfirmField.text)
                        background: Rectangle {
                            radius: 8
                            color: "transparent"
                            border.color: "white"; border.width: 1
                        }
                        contentItem: Label {
                            text: parent.text; color: "white"; anchors.centerIn: parent
                        }
                    }
                }
            }
        }
    }
}
