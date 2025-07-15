import QtQuick 2.9
import QtQuick.Controls 2.2
import QtQuick.Layouts 1.3

Page {
    id: root
    property url source
    title: qsTr("文档")
    property string htmlText: ""

    Component.onCompleted: {
        var xhr = new XMLHttpRequest()
        xhr.open("GET", source)
        xhr.onreadystatechange = function() {
            if (xhr.readyState === XMLHttpRequest.DONE) {
                htmlText = xhr.responseText
            }
        }
        xhr.send()
    }

    ScrollView {
        anchors.fill: parent
        TextArea {
            id: textArea
            text: htmlText
            readOnly: true
            wrapMode: Text.Wrap
            textFormat: TextEdit.RichText
            width: parent.width
        }
    }
}
