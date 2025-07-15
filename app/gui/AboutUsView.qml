import QtQuick 2.9
import QtQuick.Controls 2.2
import QtQuick.Layouts 1.3

Page {
    id: root
    title: qsTr("关于我们")

    Flickable {
        anchors.fill: parent
        contentHeight: content.height
        flickableDirection: Flickable.VerticalFlick

        ColumnLayout {
            id: content
            width: parent.width
            spacing: 20
            anchors.top: parent.top
            anchors.horizontalCenter: parent.horizontalCenter
            anchors.topMargin: 20

            Label {
                text: qsTr("关于我们")
                font.pixelSize: 20
                font.bold: true
                horizontalAlignment: Text.AlignHCenter
                width: parent.width
                color: "#ffffff"
            }

            Image {
                source: "qrc:/res/logo.svg"
                width: 100
                height: 100
                anchors.horizontalCenter: parent.horizontalCenter
                fillMode: Image.PreserveAspectFit
            }

            Label {
                text: qsTr("果真云电脑")
                font.pixelSize: 20
                font.bold: true
                horizontalAlignment: Text.AlignHCenter
                width: parent.width
                color: "#ffffff"
            }

            Label {
                text: qsTr("v1.0")
                horizontalAlignment: Text.AlignHCenter
                width: parent.width
                color: "#ffffff"
            }

            ColumnLayout {
                anchors.horizontalCenter: parent.horizontalCenter
                spacing: 6
                width: parent.width - 40

                Label {
                    text: qsTr("特别鸣谢")
                    font.pixelSize: 14
                    horizontalAlignment: Text.AlignHCenter
                    width: parent.width
                    color: "#ffffff"
                }
                Label {
                    text: qsTr("感谢Github用户：qiin2333提供的开源项目sunshine基地版定制")
                    wrapMode: Text.WrapAnywhere
                    color: "#ffffff"
                }
                Label {
                    text: qsTr("开源项目地址：https://github.com/qiin2333/Sunshine-Foundation")
                    wrapMode: Text.WrapAnywhere
                    color: "#ffffff"
                }
                Label {
                    text: qsTr("感谢Github用户：WACrown提供的开源项目moonlight修改版")
                    wrapMode: Text.WrapAnywhere
                    color: "#ffffff"
                }
                Label {
                    text: qsTr("开源项目地址：https://github.com/WACrown/moonlight-android")
                    wrapMode: Text.WrapAnywhere
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
}
