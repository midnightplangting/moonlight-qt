import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import UserService 1.0  // 注册的 UserService 类型

Item {
    id: root
    anchors.fill: parent

    property alias username: loginUsernameField.text
    property alias password: loginPasswordField.text
    property alias confirmPassword: registerConfirmField.text
    signal requestLogin(string username, string password)
    signal requestRegister(string username, string password, string confirmPassword)
    signal requestClose()

    property string registerErrorMessage: ""
    property string loginErrorMessage: ""

    function isPasswordValid(pwd) {
        return pwd.length >= 8 && /[A-Za-z]/.test(pwd) && /[0-9]/.test(pwd)
    }

    readonly property color borderColor: "white"

    UserService {
        id: userService

        onLoginSuccess: {
            console.log("Login success")
            loginErrorMessage = ""
            requestClose()
        }

        onLoginFailure: (errorMsg) => {
            console.log("Login failed:", errorMsg)
            loginErrorMessage = errorMsg
        }

        onRegisterSuccess: {
            console.log("Register success")
            registerErrorMessage = ""
            requestClose()
        }

        onRegisterFailure: (errorMsg) => {
            console.log("Register failed:", errorMsg)
            registerErrorMessage = errorMsg
        }
    }

    Rectangle {
        id: card
        width: 400
        height: 560
        radius: 10
        color: "#2b2b2b"
        anchors.centerIn: parent

        ColumnLayout {
            anchors.fill: parent
            anchors.margins: 20
            spacing: 20

            TabBar {
                id: tabBar
                Layout.alignment: Qt.AlignHCenter
                contentWidth: card.width * 0.5
                background: Rectangle { color: "transparent" }

                TabButton { text: qsTr("登录") }
                TabButton { text: qsTr("注册") }
            }

            StackLayout {
                id: stack
                currentIndex: tabBar.currentIndex
                width: parent.width

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

                    Label {
                        text: loginErrorMessage
                        color: "red"
                        font.pixelSize: 12
                        visible: loginErrorMessage !== ""
                    }

                    Rectangle {
                        width: parent.width
                        height: 40
                        radius: 6
                        color: "#33cc66"

                        Text {
                            text: qsTr("登录")
                            anchors.centerIn: parent
                            color: "white"
                            font.pixelSize: 16
                            font.bold: true
                        }

                        MouseArea {
                            anchors.fill: parent
                            onClicked: {
                                if (!isPasswordValid(loginPasswordField.text)) {
                                    loginErrorMessage = qsTr("密码至少8位，需包含字母和数字")
                                    return
                                }
                                userService.login(loginUsernameField.text, loginPasswordField.text)
                            }
                        }
                    }

                }

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

                    Label {
                        text: registerErrorMessage
                        color: "red"
                        font.pixelSize: 12
                        visible: registerErrorMessage !== ""
                    }
                    Rectangle {
                        width: parent.width
                        height: 40
                        radius: 6
                        color: "#33cc66"

                        Text {
                            text: qsTr("注册")
                            anchors.centerIn: parent
                            color: "white"
                            font.pixelSize: 16
                            font.bold: true
                        }

                        MouseArea {
                            anchors.fill: parent
                            onClicked: {
                                if (registerPasswordField.text !== registerConfirmField.text) {
                                    registerErrorMessage = qsTr("两次密码不一致")
                                    return
                                }
                                if (!isPasswordValid(registerPasswordField.text)) {
                                    registerErrorMessage = qsTr("密码至少8位，需包含字母和数字")
                                    return
                                }
                                userService.registerUser(
                                    registerUsernameField.text,
                                    registerPasswordField.text,
                                    registerConfirmField.text
                                )
                            }
                        }
                    }

                }
            }
        }
    }
}
