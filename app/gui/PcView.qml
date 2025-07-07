import QtQuick 2.9
import QtQuick.Controls 2.2
import QtQuick.Layouts 1.3

import ComputerModel 1.0

import ComputerManager 1.0
import StreamingPreferences 1.0
import SystemProperties 1.0
import SdlGamepadKeyNavigation 1.0
import "."

CenteredGridView {
    property ComputerModel computerModel : createModel()
    // Track the PC that is currently pairing so we can
    // automatically open its app list when pairing completes
    property int pendingPairIndex: -1
    property string pendingPairName: ""

    id: pcGrid
    focus: true
    activeFocusOnTab: true
    topMargin: 20
    bottomMargin: 5
    cellWidth: 410; cellHeight: 150;
    objectName: qsTr("Computers")

    Component.onCompleted: {
        // Don't show any highlighted item until interacting with them.
        // We do this here instead of onActivated to avoid losing the user's
        // selection when backing out of a different page of the app.
        currentIndex = -1
        // Synchronize devices once when the page is first loaded
        ComputerManager.syncOrderDevices()
    }

    // Note: Any initialization done here that is critical for streaming must
    // also be done in CliStartStreamSegue.qml, since this code does not run
    // for command-line initiated streams.
    StackView.onActivated: {
        // Setup signals on CM
        ComputerManager.computerAddCompleted.connect(addComplete)
        ComputerManager.syncOrderDevices()

        // Highlight the first item if a gamepad is connected
        if (currentIndex == -1 && SdlGamepadKeyNavigation.getConnectedGamepads() > 0) {
            currentIndex = 0
        }

    }

    Connections {
        target: ComputerManager
        onComputerStateChanged: {
            // 重新拉取设备列表，刷新 UI
            computerModel.initialize(ComputerManager)
        }
        onCloseOrderFinished: {
            if (!success) {
                errorDialog.text = message
                errorDialog.open()
            }
        }
    }

    StackView.onDeactivating: {
        ComputerManager.computerAddCompleted.disconnect(addComplete)
    }

    function pairingComplete(error)
    {
        // Close the PIN dialog and stop showing the spinner
        pairDialog.showSpinner = false
        pairDialog.close()

        // Display a failed dialog if we got an error
        if (error !== undefined) {
            errorDialog.text = error
            errorDialog.helpText = ""
            errorDialog.open()
        } else if (pendingPairIndex !== -1) {
            var component = Qt.createComponent("AppView.qml")
            var appView = component.createObject(stackView, {"computerIndex": pendingPairIndex, "objectName": pendingPairName})
            stackView.push(appView)
            pendingPairIndex = -1
            pendingPairName = ""
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

    delegate: Rectangle {
        width: 380; height: 130
        // anchors.centerIn: parent
        radius: 16
        color: "#1C1C1E"
        clip: true

        property alias pcContextMenu: pcContextMenuLoader.item
        property int index: model.index

        // 使用安全默认值避免 undefined 警告
        property string name: model && model.name !== undefined ? model.name : ""
        property real startedAt: model && model.startedAt !== undefined ? model.startedAt : 0
        property real bitrate: model && model.bitrate !== undefined ? model.bitrate : 0
        property int status: model && model.status !== undefined ? model.status : 0
        property int billingType: model && model.billingType !== undefined ? model.billingType : 0
        property int orderId: model && model.orderId !== undefined ? model.orderId : 0
        property string usageTimeText: startedAt > 0 ? Math.floor((Date.now() - startedAt)/60000).toString() : "--"
        Timer {
            id: usageTimer
            interval: 60000
            repeat: true
            running: startedAt > 0
            onTriggered: usageTimeText = startedAt > 0 ? Math.floor((Date.now() - startedAt)/60000).toString() : "--"
        }
        Component.onCompleted: if (startedAt > 0) usageTimer.start()
        property bool online: model && model.online !== undefined ? model.online : false
        property bool paired: model && model.paired !== undefined ? model.paired : false
        property bool serverSupported: model && model.serverSupported !== undefined ? model.serverSupported : false

        // 缩放动画属性
        property real hoverScale: 1.02
        property real pressScale: 0.95
        property real normalScale: 1.0
        scale: normalScale

        Behavior on scale {
            NumberAnimation {
                duration: 100
                easing.type: Easing.OutQuad
            }
        }

        MouseArea {
            id: clickArea
            anchors.fill: parent
            hoverEnabled: true
            acceptedButtons: Qt.AllButtons

            onEntered: parent.scale = hoverScale
            onExited: parent.scale = normalScale
            onPressed: parent.scale = pressScale
            onReleased: parent.scale = hoverScale
            onClicked: function(mouse) {
                if (mouse.button === Qt.RightButton) {
                    parent.pressAndHold()
                } else {
                    parent.clicked()
                }
            }
            onPressAndHold: parent.pressAndHold()
        }

        signal clicked()
        signal pressAndHold()

        focus: true
        Keys.onMenuPressed: pcContextMenu.open()
        Keys.onDeletePressed: {
            deletePcDialog.pcIndex = index
            deletePcDialog.pcName = model.name
            deletePcDialog.open()
        }

        Row {
            anchors.fill: parent
            anchors.margins: 12
            spacing: 16

            Column {
                anchors.verticalCenter: parent.verticalCenter
                spacing: 4
                Layout.fillWidth: true

                Row {
                    spacing: 6
                    Text {
                        text: model.name
                        font.pixelSize: 22
                        font.bold: true
                        color: "#FFFFFF"
                    }
                    Rectangle {
                        visible: model.online !== undefined
                        color: "#FFA54F"
                        radius: 4
                        height: 20
                        anchors.verticalCenter: parent.verticalCenter
                        implicitWidth: statusText.paintedWidth + 16

                        Text {
                            id: statusText
                            text: model.online ? qsTr("在线") : qsTr("离线")
                            font.pixelSize: 13
                            color: "#FFFFFF"
                            anchors.centerIn: parent
                            font.bold: true
                        }
                    }
                }

                Text {
                    text: qsTr("使用时长：%1 分钟").arg(usageTimeText)
                    font.pixelSize: 18
                    color: "#CCCCCC"
                }

                Text {
                    text: qsTr("串流码率：%1 Mbps").arg(bitrate)
                    font.pixelSize: 18
                    color: "#CCCCCC"
                }
            }

            Rectangle {
                id: actionBtn
                width: 40; height: 28
                color: "transparent"
                radius: 4
                anchors.verticalCenter: parent.verticalCenter

                property real normalScale: 1.0
                property real pressedScale: 0.95
                scale: normalScale
                Behavior on scale {
                    NumberAnimation { duration: 100; easing.type: Easing.OutQuad }
                }

                Text {
                    anchors.centerIn: parent
                    text: (billingType >= 2 && billingType <= 4) ? qsTr("续费") : qsTr("结账下机")
                    font.pixelSize: 20
                    color: "#007AFF"
                }

                MouseArea {
                    anchors.fill: parent
                    onPressed:  actionBtn.scale = actionBtn.pressedScale
                    onReleased: actionBtn.scale = actionBtn.normalScale
                    onClicked: {
                        if (billingType >= 2 && billingType <= 4) {
                            renewDialog.orderId = orderId
                            renewDialog.deviceGroupId = ComputerManager.getDeviceGroupIdByOrderId(orderId)
                            renewDialog.open()
                        } else {
                            computerModel.checkoutComputer(index)
                        }
                    }
                }
            }
        }

        Loader {
            id: pcContextMenuLoader
            asynchronous: true
            sourceComponent: NavigableMenu {
                id: pcContextMenu
                MenuItem {
                    text: qsTr("PC Status: %1").arg(model.online ? qsTr("Online") : qsTr("Offline"))
                    font.bold: true
                    enabled: false
                }
                NavigableMenuItem {
                    parentMenu: pcContextMenu
                    text: qsTr("View All Apps")
                    onTriggered: {
                        var component = Qt.createComponent("AppView.qml")
                        var appView = component.createObject(stackView, {"computerIndex": index, "objectName": model.name, "showHiddenGames": true})
                        stackView.push(appView)
                    }
                    visible: model.online && model.paired
                }
                NavigableMenuItem {
                    parentMenu: pcContextMenu
                    text: qsTr("Wake PC")
                    onTriggered: computerModel.wakeComputer(index)
                    visible: !model.online && model.wakeable
                }
                NavigableMenuItem {
                    parentMenu: pcContextMenu
                    text: qsTr("Test Network")
                    onTriggered: {
                        computerModel.testConnectionForComputer(index)
                        testConnectionDialog.open()
                    }
                }

                NavigableMenuItem {
                    parentMenu: pcContextMenu
                    text: qsTr("Rename PC")
                    onTriggered: {
                        renamePcDialog.pcIndex = index
                        renamePcDialog.originalName = model.name
                        renamePcDialog.open()
                    }
                }
                NavigableMenuItem {
                    parentMenu: pcContextMenu
                    text: qsTr("Delete PC")
                    onTriggered: {
                        deletePcDialog.pcIndex = index
                        deletePcDialog.pcName = model.name
                        deletePcDialog.open()
                    }
                }
                NavigableMenuItem {
                    parentMenu: pcContextMenu
                    text: qsTr("View Details")
                    onTriggered: {
                        showPcDetailsDialog.pcDetails = model.details
                        showPcDetailsDialog.open()
                    }
                }
            }
        }
        onClicked: {
                if (model.online) {
                    if (bitrate > 0) {
                        StreamingPreferences.bitrateKbps = bitrate * 1000
                        StreamingPreferences.autoAdjustBitrate = false
                    }
                    var result = computerModel.handlePcClicked(index)
                    if (result.error !== undefined) {
                        errorDialog.text = result.error
                        errorDialog.open()
                    } else if (result.open) {
                        var component = Qt.createComponent("AppView.qml")
                        var appView = component.createObject(stackView, {"computerIndex": index, "objectName": model.name})
                        stackView.push(appView)
                    } else {
                        pendingPairIndex = index
                        pendingPairName = model.name
                        pairDialog.pin = result.pin !== undefined ? result.pin : ""
                        pairDialog.showSpinner = true
                        pairDialog.open()
                    }
                } else {
                    pcContextMenu.open()
                }
            }

        onPressAndHold: {
            if (pcContextMenu.popup)
                pcContextMenu.popup()
            else
                pcContextMenu.open()
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
        closePolicy: Popup.NoAutoClose

        // don't allow edits to the rest of the window while open
        property string pin : ""
        text: pin !== "" ?
                 qsTr("Please enter %1 on your host PC. This dialog will close when pairing is completed.").arg(pin) + "\n\n" +
                 qsTr("If your host PC is running Sunshine, navigate to the Sunshine web UI to enter the PIN.") :
                 qsTr("Pairing with your PC. Please wait…")
        showSpinner: true
        standardButtons: Dialog.NoButton
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

    RenewDialog { id: renewDialog }

    ScrollBar.vertical: ScrollBar {}
}
