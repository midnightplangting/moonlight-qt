import QtQuick 2.9
import QtQuick.Controls 2.2
import QtQuick.Layouts 1.3
import ComputerManager 1.0

NavigableDialog {
    id: root
    property int orderId: 0
    property int deviceGroupId: 0
    property real hourlyPrice: 0
    property real dayPrice: 0
    property real weekPrice: 0
    property real monthPrice: 0
    property int unitIndex: 0 // 0=小时 1=天 2=周 3=月
    property int quantity: 1

    title: qsTr("请选择续费方式")
    standardButtons: Dialog.Cancel | Dialog.Ok

    function updatePrices() {
        for (var i = 0; i < gpuModel.count; i++) {
            var item = gpuModel.get(i)
            if (item.groupId === deviceGroupId) {
                hourlyPrice = item.hourly
                dayPrice = item.day
                weekPrice = item.week
                monthPrice = item.month
                break
            }
        }
    }

    onDeviceGroupIdChanged: updatePrices()

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 12
        spacing: 12

        Label {
            text: qsTr("共计%1金币").arg((unitIndex === 0 ? hourlyPrice : unitIndex === 1 ? dayPrice : unitIndex === 2 ? weekPrice : monthPrice) * quantity)
            Layout.alignment: Qt.AlignHCenter
        }

        RowLayout {
            Layout.alignment: Qt.AlignHCenter
            spacing: 8
            Repeater {
                model: [qsTr("小时"), qsTr("天"), qsTr("周"), qsTr("月")]
                delegate: Button {
                    text: modelData
                    checkable: true
                    checked: index === unitIndex
                    onClicked: unitIndex = index
                }
            }
        }

        RowLayout {
            Layout.alignment: Qt.AlignHCenter
            spacing: 8
            Button {
                text: "-"
                enabled: quantity > 1
                onClicked: quantity--
            }
            Label { text: quantity }
            Button {
                text: "+"
                onClicked: quantity++
            }
        }
    }

    onAccepted: {
        var typeMap = [1,2,3,4]
        ComputerManager.allocateDevice(orderId, typeMap[unitIndex])
    }
}
