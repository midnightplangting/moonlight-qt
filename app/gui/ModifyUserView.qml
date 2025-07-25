import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import UserSession 1.0
import ComputerManager 1.0
import UserService 1.0

Item {
    id: overlayRoot
    width: 400
    height: 560
    anchors.centerIn: parent
    visible: true
    z: 999

    property string pendingField: ""

    function isPasswordValid(pwd) {
        return pwd.length >= 8 && /[A-Za-z]/.test(pwd) && /[0-9]/.test(pwd)
    }

    UserService {
        id: userService
        onUpdateUserInfoSuccess: {
            console.log("update success", msg)
            if (pendingField === "username") {
                UserSession.username = usernameField.text
                UserSession.saveToSettings()
            }
            pendingField = ""
            selectedIndex = -1
        }
        onUpdateUserInfoFailure: {
            console.warn("update failed", errorMsg)
            pendingField = ""
        }
    }

    signal requestClose()

    property int selectedIndex: -1  // -1=菜单, 0=用户名, 1=手机号, 2=邮箱, 3=密码

    Rectangle {
        id: dialog
        width: 400
        height: 560
        radius: 10
        color: "#2b2b2b"
        anchors.centerIn: parent

        ColumnLayout {
            anchors.fill: parent
            anchors.margins: 20
            spacing: 20

            Label {
                text: selectedIndex === -1 ? "修改用户资料" : "返回菜单"
                color: "white"
                font.pixelSize: 20
                Layout.alignment: Qt.AlignHCenter
            }

            // 菜单按钮组（仅在未选中具体操作时显示）
            ColumnLayout {
                visible: selectedIndex === -1
                spacing: 10

                Rectangle {
                    height: 40; Layout.fillWidth: true; radius: 8; color: "#3a3a3c"
                    MouseArea {
                        anchors.fill: parent
                        onClicked: selectedIndex = 0
                    }
                    Text {
                        text: qsTr("修改用户名")
                        anchors.centerIn: parent
                        color: "white"
                    }
                }
                Rectangle {
                    height: 40; Layout.fillWidth: true; radius: 8; color: "#3a3a3c"
                    MouseArea {
                        anchors.fill: parent
                        onClicked: selectedIndex = 1
                    }
                    Text {
                        text: qsTr("修改手机号")
                        anchors.centerIn: parent
                        color: "white"
                    }
                }
                Rectangle {
                    height: 40; Layout.fillWidth: true; radius: 8; color: "#3a3a3c"
                    MouseArea {
                        anchors.fill: parent
                        onClicked: selectedIndex = 2
                    }
                    Text {
                        text: qsTr("修改邮箱")
                        anchors.centerIn: parent
                        color: "white"
                    }
                }
                Rectangle {
                    height: 40; Layout.fillWidth: true; radius: 8; color: "#3a3a3c"
                    MouseArea {
                        anchors.fill: parent
                        onClicked: selectedIndex = 3
                    }
                    Text {
                        text: qsTr("修改密码")
                        anchors.centerIn: parent
                        color: "white"
                    }
                }
                Rectangle {
                    height: 40; Layout.fillWidth: true; radius: 8; color: "#ff3b30"
                    MouseArea {
                        anchors.fill: parent
                        onClicked: {
                            UserSession.logout()
                            ComputerManager.clearOrderDevices()
                            overlayRoot.requestClose()
                        }
                    }
                    Text {
                        text: qsTr("退出登录")
                        anchors.centerIn: parent
                        color: "white"
                        font.bold: true
                    }
                }
            }

            // 输入表单切换区域（选中后显示对应表单）
            Item {
                Layout.fillWidth: true
                Layout.fillHeight: true

                ColumnLayout {
                    anchors.fill: parent
                    spacing: 6
                    Layout.alignment: Qt.AlignTop
                    visible: selectedIndex === 0

                    TextField { id: usernameField; placeholderText: "新用户名"; Layout.fillWidth: true }
                    Item { Layout.fillHeight: true }
                    Rectangle {
                        height: 40; Layout.fillWidth: true; radius: 6; color: "#33cc66"
                        Text {
                            text: "确认修改"
                            anchors.centerIn: parent
                            color: "white"
                        }
                        MouseArea {
                            anchors.fill: parent
                            onClicked: {
                                pendingField = "username"
                                userService.updateUserInfo("username", usernameField.text)
                            }
                        }
                    }
                }

                ColumnLayout {
                    anchors.fill: parent
                    spacing: 6
                    Layout.alignment: Qt.AlignTop
                    visible: selectedIndex === 1

                    TextField { id: phoneField; placeholderText: "新手机号"; Layout.fillWidth: true }
                    Item { Layout.fillHeight: true }
                    Rectangle {
                        height: 40; Layout.fillWidth: true; radius: 6; color: "#33cc66"
                        Text {
                            text: "确认修改"
                            anchors.centerIn: parent
                            color: "white"
                        }
                        MouseArea {
                            anchors.fill: parent
                            onClicked: {
                                pendingField = "phone"
                                userService.updateUserInfo("phone", phoneField.text)
                            }
                        }
                    }
                }

                ColumnLayout {
                    anchors.fill: parent
                    spacing: 6
                    Layout.alignment: Qt.AlignTop
                    visible: selectedIndex === 2

                    TextField { id: emailField; placeholderText: "新邮箱"; Layout.fillWidth: true }
                    Item { Layout.fillHeight: true }
                    Rectangle {
                        height: 40; Layout.fillWidth: true; radius: 6; color: "#33cc66"
                        Text {
                            text: "确认修改"
                            anchors.centerIn: parent
                            color: "white"
                        }
                        MouseArea {
                            anchors.fill: parent
                            onClicked: {
                                pendingField = "email"
                                userService.updateUserInfo("email", emailField.text)
                            }
                        }
                    }
                }

                ColumnLayout {
                    anchors.fill: parent
                    spacing: 6
                    Layout.alignment: Qt.AlignTop
                    visible: selectedIndex === 3

                    TextField { id: oldPasswordField; placeholderText: "原密码"; echoMode: TextInput.Password; Layout.fillWidth: true }
                    TextField { id: newPasswordField; placeholderText: "新密码"; echoMode: TextInput.Password; Layout.fillWidth: true }
                    TextField { id: confirmPasswordField; placeholderText: "确认新密码"; echoMode: TextInput.Password; Layout.fillWidth: true }
                    Item { Layout.fillHeight: true }
                    Rectangle {
                        height: 40; Layout.fillWidth: true; radius: 6; color: "#33cc66"
                        Text {
                            text: "确认修改"
                            anchors.centerIn: parent
                            color: "white"
                        }
                        MouseArea {
                            anchors.fill: parent
                            onClicked: {
                                if (newPasswordField.text !== confirmPasswordField.text) {
                                    console.warn("两次密码不一致")
                                    return
                                }
                                if (!isPasswordValid(newPasswordField.text)) {
                                    console.warn("密码至少8位，需包含字母和数字")
                                    return
                                }
                                pendingField = "password"
                                userService.updateUserInfo("password", newPasswordField.text)
                            }
                        }
                    }
                }
            }

            // 取消返回菜单按钮（表单显示时）
            Rectangle {
                visible: selectedIndex !== -1
                height: 40; Layout.fillWidth: true; radius: 6; color: "#666666"
                Text {
                    text: "取消"
                    anchors.centerIn: parent
                    color: "white"
                }
                MouseArea {
                    anchors.fill: parent
                    onClicked: selectedIndex = -1
                }
            }
        }
    }
}
