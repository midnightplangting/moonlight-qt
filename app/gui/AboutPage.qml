import QtQuick 2.9
import QtQuick.Controls 2.2
import QtQuick.Layouts 1.3

Page {
    id: aboutPage
    objectName: "AboutPage"

    title: qsTr("About")

    ColumnLayout {
        anchors.centerIn: parent
        spacing: 20

        Label {
            text: qsTr("Moonlight-Qt 自定义页面")
            font.pointSize: 20
            horizontalAlignment: Qt.AlignHCenter
        }

        Label {
            text: qsTr("版本号: 1.0\n作者: 你自己")
            horizontalAlignment: Qt.AlignHCenter
        }

        Button {
            text: qsTr("返回")
            onClicked: stackView.pop()
        }
    }
}
