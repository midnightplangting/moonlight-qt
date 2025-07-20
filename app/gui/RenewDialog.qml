import QtQuick 2.9
import QtQuick.Controls 2.2
import QtQuick.Layouts 1.3
import ComputerManager 1.0

NavigableDialog {
    id: root
    property int orderId: 0
    property int deviceGroupId: 0
    property real dayPrice: 0
    property real weekPrice: 0
    property real monthPrice: 0
    property real timingPrice: 0
    property real unitPrice: 0
    property real totalPrice: 0
    property int unitIndex: 0 // 0=天 1=周 2=月 3=包时
    property int quantity: 1

    title: qsTr("请选择续费方式")
    standardButtons: Dialog.Cancel | Dialog.Ok

    function updatePrices() {
        var item = gpuModel.getGroup(deviceGroupId)
        if (item && item.day !== undefined) {
            dayPrice = item.day
            weekPrice = item.week
            monthPrice = item.month
            timingPrice = item.hourly
            console.log("[RenewDialog] pricing", deviceGroupId, dayPrice, weekPrice, monthPrice, timingPrice)
        } else {
            console.log("[RenewDialog] no pricing for", deviceGroupId)
            dayPrice = 0; weekPrice = 0; monthPrice = 0; timingPrice = 0
        }
        recalcTotal()
    }

    function recalcTotal() {
        switch (unitIndex) {
        case 0: unitPrice = dayPrice; break
        case 1: unitPrice = weekPrice; break
        case 2: unitPrice = monthPrice; break
        case 3: unitPrice = timingPrice; break
        }
        totalPrice = unitPrice * quantity
        console.log("[RenewDialog] price", unitPrice, "qty", quantity, "total", totalPrice)
    }

    onDeviceGroupIdChanged: updatePrices()
    onUnitIndexChanged: recalcTotal()
    onQuantityChanged: recalcTotal()
    onOpened: {
        console.log("[RenewDialog] open", orderId, deviceGroupId)
        gpuModel.refresh()
        updatePrices()
    }

    Connections {
        target: gpuModel
        onModelReset: updatePrices()
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 12
        spacing: 12

        Label {
            text: qsTr("共计%1金币").arg(totalPrice)
            Layout.alignment: Qt.AlignHCenter
        }

        RowLayout {
            Layout.alignment: Qt.AlignHCenter
            spacing: 8
            Repeater {
                model: [qsTr("包天"), qsTr("包周"), qsTr("包月"), qsTr("包时")]
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
        var typeMap = [2,3,4,7]
        console.log("[RenewDialog] recharge", orderId, quantity, typeMap[unitIndex])
        loadingDialog.open()
        ComputerManager.rechargeOrder(orderId, quantity, typeMap[unitIndex])
    }

    NavigableMessageDialog {
        id: loadingDialog
        modal: true
        closePolicy: Popup.NoAutoClose
        text: qsTr("正在续费，请稍候…")
        showSpinner: true
        standardButtons: Dialog.NoButton
    }

    NavigableMessageDialog {
        id: resultDialog
        standardButtons: Dialog.Ok
    }

    Connections {
        target: ComputerManager
        onRechargeOrderFinished: function(success, msg) {
            loadingDialog.close()
            resultDialog.text = msg
            resultDialog.open()
            if (success)
                root.close()
        }
    }
}
