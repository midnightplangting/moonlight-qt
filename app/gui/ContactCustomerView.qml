import QtQuick 2.9
import QtQuick.Controls 2.2
import QtQuick.Layouts 1.3

Page {
    id: root
    title: qsTr("联系客服")

    ColumnLayout {
        anchors.centerIn: parent
        anchors.margins: 20
        spacing: 20
        width: parent.width

        Label {
            text: qsTr("联系客服")
            font.pixelSize: 20
            font.bold: true
            horizontalAlignment: Text.AlignHCenter
            width: parent.width
            color: "#ffffff"
        }

        RowLayout {
            spacing: 10
            anchors.horizontalCenter: parent.horizontalCenter

            Label {
                text: qsTr("请添加客服的微信号:")
                font.pixelSize: 16
                color: "#ffffff"
            }
            Label {
                text: "zxbfy52"
                font.pixelSize: 18
                font.bold: true
                color: "#ffffff"
            }
        }

        Button {
            text: qsTr("返回")
            anchors.horizontalCenter: parent.horizontalCenter
            onClicked: stackView.pop()
        }
    }
}
