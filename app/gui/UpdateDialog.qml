import QtQuick 2.9
import QtQuick.Controls 2.2
import QtQuick.Layouts 1.3

// Dialog prompting the user to download and install an update.
NavigableDialog {
    id: root
    width: 400

    property string message: qsTr("有最新的软件包，请下载并安装!")
    property string packageUrl: ""
    property real progress: 0
    property string downloadedFile: ""
    property bool readyToInstall: false

    property int cancelLeftMargin: 12
    property int actionRightMargin: 12
    property int buttonBottomPadding: 5

    title: qsTr("软件版本更新")
    standardButtons: Dialog.NoButton

    function startDownload() {
        progressBar.visible = true
        cancelButton.visible = false
        actionButton.visible = false
        userService.downloadUpdate(packageUrl)
    }

    function showInstallButton() {
        progressBar.visible = false
        readyToInstall = true
        cancelButton.visible = true
        actionButton.visible = true
    }

    ColumnLayout {
        spacing: 10
        width: parent.width
        anchors.horizontalCenter: parent.horizontalCenter

        Label {
            text: root.message
            wrapMode: Text.Wrap
            elide: Text.ElideNone
            horizontalAlignment: Text.AlignHCenter
            Layout.fillWidth: true
            font.pixelSize: 14
        }

        ProgressBar {
            id: progressBar
            from: 0
            to: 1
            value: root.progress
            visible: false
            Layout.fillWidth: true
        }

        RowLayout {
            Layout.fillWidth: true
            spacing: 20

            Button {
                id: cancelButton
                text: qsTr("以后再说")
                onClicked: root.close()
            }

            Item { Layout.fillWidth: true }  // 占位符推开两边按钮

            Button {
                id: actionButton
                text: root.readyToInstall ? qsTr("安装!") : qsTr("立即下载")
                onClicked: {
                    if (root.readyToInstall) {
                        userService.installUpdate(root.downloadedFile)
                    } else {
                        root.startDownload()
                    }
                }
            }
        }
    }
}
