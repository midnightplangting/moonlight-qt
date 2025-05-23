import QtQuick 2.9
import QtQuick.Controls 2.2
import QtQuick.Layouts 1.3

import ComputerModel 1.0

import ComputerManager 1.0
import StreamingPreferences 1.0
import SystemProperties 1.0
import SdlGamepadKeyNavigation 1.0

CenteredGridView {
    property ComputerModel computerModel : createModel()

    id: pcGrid
    focus: true
    activeFocusOnTab: true
    topMargin: 20
    bottomMargin: 5
    cellWidth: 310; cellHeight: 330;
    objectName: qsTr("Computers")

    Component.onCompleted: {
        // Don't show any highlighted item until interacting with them.
        // We do this here instead of onActivated to avoid losing the user's
        // selection when backing out of a different page of the app.
        currentIndex = -1
    }

    // Note: Any initialization done here that is critical for streaming must
    // also be done in CliStartStreamSegue.qml, since this code does not run
    // for command-line initiated streams.
    StackView.onActivated: {
        // Setup signals on CM
        ComputerManager.computerAddCompleted.connect(addComplete)

        // Highlight the first item if a gamepad is connected
        if (currentIndex == -1 && SdlGamepadKeyNavigation.getConnectedGamepads() > 0) {
            currentIndex = 0
        }
    }

    StackView.onDeactivating: {
        ComputerManager.computerAddCompleted.disconnect(addComplete)
    }

    function pairingComplete(error)
    {
        // Close the PIN dialog
        pairDialog.close()

        // Display a failed dialog if we got an error
        if (error !== undefined) {
            errorDialog.text = error
            errorDialog.helpText = ""
            errorDialog.open()
        }
    }

    function addComplete(success, detectedPortBlocking)
    {
        if (!success) {
            errorDialog.text = qsTr("Unable to connect to the specified PC.")

            if (detectedPortBlocking) {
                errorDialog.text += "\n\n" + qsTr("This PC's Internet connection is blocking Moonlight. Streaming over the Internet may not work while connected to this network.")
            }
            else {
                errorDialog.helpText = qsTr("Click the Help button for possible solutions.")
            }

            errorDialog.open()
        }
    }

    function createModel()
    {
        var model = Qt.createQmlObject('import ComputerModel 1.0; ComputerModel {}', parent, '')
        model.initialize(ComputerManager)
        model.pairingCompleted.connect(pairingComplete)
        model.connectionTestCompleted.connect(testConnectionDialog.connectionTestComplete)
        return model
    }

    Row {
        anchors.centerIn: parent
        spacing: 5
        visible: pcGrid.count === 0

        BusyIndicator {
            id: searchSpinner
            visible: StreamingPreferences.enableMdns
        }

        Label {
            height: searchSpinner.height
            elide: Label.ElideRight
            text: StreamingPreferences.enableMdns ? qsTr("Searching for compatible hosts on your local network...")
                                                  : qsTr("Automatic PC discovery is disabled. Add your PC manually.")
            font.pointSize: 20
            verticalAlignment: Text.AlignVCenter
            wrapMode: Text.Wrap
        }
    }

    model: computerModel

    delegate: NavigableItemDelegate {
        width: 330; height: 100
        grid: pcGrid

        // 用于控制按下状态
        property bool pressed: false

        // 1. 背景圆角长方形 + 动画
        Rectangle {
            id: background
            anchors.fill: parent
            radius: 8
            // 默认色 和 按下色
            property color normalColor: "#2C2C2E"
            property color pressedColor: "#444448"
            color: parent.pressed ? pressedColor : normalColor

            // 颜色过渡动画
            Behavior on color { ColorAnimation { duration: 150; easing.type: Easing.OutQuad } }
            // 缩放过渡动画
            Behavior on scale { NumberAnimation { duration: 100; easing.type: Easing.OutQuad } }
            // 默认不缩放
            scale: parent.pressed ? 0.97 : 1.0
        }

        Row {
            anchors.fill: parent
            anchors.margins: 12
            spacing: 16

            // 1. 左侧图标
            Image {
                id: pcIcon
                source: "qrc:/res/desktop_windows-48px.svg"
                width: 24; height: 24
                fillMode: Image.PreserveAspectFit
                anchors.verticalCenter: parent.verticalCenter
            }

            // 2. 中间信息列
            Column {
                anchors.verticalCenter: parent.verticalCenter
                spacing: 4
                Layout.fillWidth: true

                // 2.1 机器名 + 状态徽章
                Row {
                    spacing: 6
                    // 机器名
                    Text {
                        text: model.name
                        font.pixelSize: 16
                        font.bold: true
                        color: "#FFFFFF"
                    }
                    // 状态徽章
                    Rectangle {
                        visible: model.online !== undefined
                        color: "#FFD43A"
                        radius: 4
                        height: 20
                        anchors.verticalCenter: parent.verticalCenter
                        // 宽度根据文字自动撑开
                        implicitWidth: statusText.paintedWidth + 16

                        Text {
                            id: statusText
                            text: model.online ? qsTr("在线") : qsTr("离线")
                            font.pixelSize: 12
                            color: "#FFFFFF"
                            anchors.centerIn: parent
                        }
                    }
                }

                // 2.2 使用时长
                Text {
                    text: qsTr("使用时长：%1").arg(model.usageTime)
                    font.pixelSize: 14
                    color: "#CCCCCC"
                }
                // 2.3 串流码率
                Text {
                    text: qsTr("串流码率：%1 Mbps").arg(model.bitrate)
                    font.pixelSize: 14
                    color: "#CCCCCC"
                }
            }

            // 3. 右侧操作按钮
            Rectangle {
                id: actionBtn
                width: 60; height: 28
                color: "transparent"
                radius: 4
                anchors.verticalCenter: parent.verticalCenter

                // 缩放效果
                property real normalScale: 1.0
                property real pressedScale: 0.9
                scale: normalScale
                Behavior on scale {
                    NumberAnimation { duration: 100; easing.type: Easing.OutQuad }
                }

                // 按钮文字
                Text {
                    anchors.centerIn: parent
                    text: qsTr("结账下机")      // 或者根据 model 来决定显示 “关机” 等
                    font.pixelSize: 14
                    color: "#007AFF"
                }

                // 点击交互
                MouseArea {
                    anchors.fill: parent
                    onPressed:  actionBtn.scale = actionBtn.pressedScale
                    onReleased: actionBtn.scale = actionBtn.normalScale
                    onClicked: {
                        // TODO: 在这里处理“结账”或“关机”等逻辑
                        console.log("action clicked for", model.name)
                    }
                }
            }
        }
    }


    ErrorMessageDialog {
        id: errorDialog

        // Using Setup-Guide here instead of Troubleshooting because it's likely that users
        // will arrive here by forgetting to enable GameStream or not forwarding ports.
        helpUrl: "https://github.com/moonlight-stream/moonlight-docs/wiki/Setup-Guide"
    }

    NavigableMessageDialog {
        id: pairDialog

        // Pairing dialog must be modal to prevent double-clicks from triggering
        // pairing twice
        modal: true
        closePolicy: Popup.CloseOnEscape

        // don't allow edits to the rest of the window while open
        property string pin : "0000"
        text:qsTr("Please enter %1 on your host PC. This dialog will close when pairing is completed.").arg(pin)+"\n\n"+
             qsTr("If your host PC is running Sunshine, navigate to the Sunshine web UI to enter the PIN.")
        standardButtons: Dialog.Cancel
        onRejected: {
            // FIXME: We should interrupt pairing here
        }
    }

    NavigableMessageDialog {
        id: deletePcDialog
        // don't allow edits to the rest of the window while open
        property int pcIndex : -1
        property string pcName : ""
        text: qsTr("Are you sure you want to remove '%1'?").arg(pcName)
        standardButtons: Dialog.Yes | Dialog.No

        onAccepted: {
            computerModel.deleteComputer(pcIndex)
        }
    }

    NavigableMessageDialog {
        id: testConnectionDialog
        closePolicy: Popup.CloseOnEscape
        standardButtons: Dialog.Ok

        onAboutToShow: {
            testConnectionDialog.text = qsTr("Moonlight is testing your network connection to determine if any required ports are blocked.") + "\n\n" + qsTr("This may take a few seconds…")
            showSpinner = true
        }

        function connectionTestComplete(result, blockedPorts)
        {
            if (result === -1) {
                text = qsTr("The network test could not be performed because none of Moonlight's connection testing servers were reachable from this PC. Check your Internet connection or try again later.")
                imageSrc = "qrc:/res/baseline-warning-24px.svg"
            }
            else if (result === 0) {
                text = qsTr("This network does not appear to be blocking Moonlight. If you still have trouble connecting, check your PC's firewall settings.") + "\n\n" + qsTr("If you are trying to stream over the Internet, install the Moonlight Internet Hosting Tool on your gaming PC and run the included Internet Streaming Tester to check your gaming PC's Internet connection.")
                imageSrc = "qrc:/res/baseline-check_circle_outline-24px.svg"
            }
            else {
                text = qsTr("Your PC's current network connection seems to be blocking Moonlight. Streaming over the Internet may not work while connected to this network.") + "\n\n" + qsTr("The following network ports were blocked:") + "\n"
                text += blockedPorts
                imageSrc = "qrc:/res/baseline-error_outline-24px.svg"
            }

            // Stop showing the spinner and show the image instead
            showSpinner = false
        }
    }

    NavigableDialog {
        id: renamePcDialog
        property string label: qsTr("Enter the new name for this PC:")
        property string originalName
        property int pcIndex : -1;

        standardButtons: Dialog.Ok | Dialog.Cancel

        onOpened: {
            // Force keyboard focus on the textbox so keyboard navigation works
            editText.forceActiveFocus()
        }

        onClosed: {
            editText.clear()
        }

        onAccepted: {
            if (editText.text) {
                computerModel.renameComputer(pcIndex, editText.text)
            }
        }

        ColumnLayout {
            Label {
                text: renamePcDialog.label
                font.bold: true
            }

            TextField {
                id: editText
                placeholderText: renamePcDialog.originalName
                Layout.fillWidth: true
                focus: true

                Keys.onReturnPressed: {
                    renamePcDialog.accept()
                }

                Keys.onEnterPressed: {
                    renamePcDialog.accept()
                }
            }
        }
    }

    NavigableMessageDialog {
        id: showPcDetailsDialog
        property string pcDetails : "";
        text: showPcDetailsDialog.pcDetails
        imageSrc: "qrc:/res/baseline-help_outline-24px.svg"
        standardButtons: Dialog.Ok
    }

    ScrollBar.vertical: ScrollBar {}
}
