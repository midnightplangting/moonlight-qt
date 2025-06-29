import QtQuick 2.9
import QtQuick.Controls 2.2
import ComputerManager 1.0

NavigableDialog {
    id: root
    property int orderId: 0
    title: qsTr("选择续费时长")
    standardButtons: Dialog.Cancel
    Column {
        spacing: 12
        anchors.margins: 12
        anchors.fill: parent
        Button { text: qsTr("包天"); onClicked: { ComputerManager.allocateDevice(orderId, 2); root.close() } }
        Button { text: qsTr("包周"); onClicked: { ComputerManager.allocateDevice(orderId, 3); root.close() } }
        Button { text: qsTr("包月"); onClicked: { ComputerManager.allocateDevice(orderId, 4); root.close() } }
    }
}
