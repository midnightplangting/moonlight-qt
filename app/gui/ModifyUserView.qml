import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import UserSession 1.0

Item {
    id: root
    anchors.fill: parent
    property color bgColor: "#2b2b2b"

    Rectangle {
        anchors.fill: parent
        color: bgColor
    }

    ColumnLayout {
        anchors.centerIn: parent
        spacing: 20

        Label {
            text: qsTr("修改用户信息")
            font.pixelSize: 24
            color: "white"
            horizontalAlignment: Text.AlignHCenter
            Layout.alignment: Qt.AlignHCenter
        }

        // 用户名修改
        Button {
            text: qsTr("修改用户名")
            onClicked: {
                usernamePopup.x = (root.width - usernamePopup.width) / 2
                usernamePopup.y = (root.height - usernamePopup.height) / 2
                usernamePopup.open()
            }
        }

        // 手机号修改
        Button {
            text: qsTr("修改手机号")
            onClicked: {
                phonePopup.x = (root.width - phonePopup.width) / 2
                phonePopup.y = (root.height - phonePopup.height) / 2
                phonePopup.open()
            }
        }

        // 邮箱修改
        Button {
            text: qsTr("修改邮箱")
            onClicked: {
                emailPopup.x = (root.width - emailPopup.width) / 2
                emailPopup.y = (root.height - emailPopup.height) / 2
                emailPopup.open()
            }
        }

        // 密码修改
        Button {
            text: qsTr("修改密码")
            onClicked: {
                passwordPopup.x = (root.width - passwordPopup.width) / 2
                passwordPopup.y = (root.height - passwordPopup.height) / 2
                passwordPopup.open()
            }
        }

        // 退出登录
        Button {
            text: qsTr("退出登录")
            background: Rectangle { color: "#ff4444"; radius: 4 }
            onClicked: {
                UserSession.logout()
                stackView.pop()
            }
        }
    }

    // 用户名输入弹窗
    Popup {
        id: usernamePopup
        modal: true
        focus: true
        closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutside
        width: 300; height: 150
        contentItem: Rectangle {
            anchors.fill: parent
            color: "#333333"; radius: 8
            ColumnLayout {
                anchors.centerIn: parent
                spacing: 10
                TextField { id: usernameField; placeholderText: "新用户名"; width: 200 }
                Button {
                    text: qsTr("确认修改")
                    onClicked: {
                        UserSession.username = usernameField.text
                        UserSession.saveToSettings()
                        usernamePopup.close()
                    }
                }
            }
        }
    }

    // 手机号输入弹窗
    Popup {
        id: phonePopup
        modal: true
        focus: true
        closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutside
        width: 300; height: 150
        contentItem: Rectangle {
            anchors.fill: parent
            color: "#333333"; radius: 8
            ColumnLayout {
                anchors.centerIn: parent
                spacing: 10
                TextField { id: phoneField; placeholderText: "新手机号"; width: 200 }
                Button {
                    text: qsTr("确认修改")
                    onClicked: {
                        console.log("手机号修改为：", phoneField.text)
                        phonePopup.close()
                        // TODO: 调用 API 更新
                    }
                }
            }
        }
    }

    // 邮箱输入弹窗
    Popup {
        id: emailPopup
        modal: true
        focus: true
        closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutside
        width: 300; height: 150
        contentItem: Rectangle {
            anchors.fill: parent
            color: "#333333"; radius: 8
            ColumnLayout {
                anchors.centerIn: parent
                spacing: 10
                TextField { id: emailField; placeholderText: "新邮箱"; width: 200 }
                Button {
                    text: qsTr("确认修改")
                    onClicked: {
                        console.log("邮箱修改为：", emailField.text)
                        emailPopup.close()
                        // TODO: 调用 API 更新
                    }
                }
            }
        }
    }

    // 密码输入弹窗
    Popup {
        id: passwordPopup
        modal: true
        focus: true
        closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutside
        width: 300; height: 220
        contentItem: Rectangle {
            anchors.fill: parent
            color: "#333333"; radius: 8
            ColumnLayout {
                anchors.centerIn: parent
                spacing: 10
                TextField {
                    id: oldPasswordField
                    placeholderText: "原密码"
                    echoMode: TextInput.Password
                    width: 200
                }
                TextField {
                    id: newPasswordField
                    placeholderText: "新密码"
                    echoMode: TextInput.Password
                    width: 200
                }
                TextField {
                    id: confirmPasswordField
                    placeholderText: "确认新密码"
                    echoMode: TextInput.Password
                    width: 200
                }
                Button {
                    text: qsTr("确认修改")
                    onClicked: {
                        if (newPasswordField.text !== confirmPasswordField.text) {
                            console.warn("两次密码不一致")
                            return
                        }
                        console.log("密码修改成功")
                        passwordPopup.close()
                        // TODO: 调用 API 更新
                    }
                }
            }
        }
    }
}
