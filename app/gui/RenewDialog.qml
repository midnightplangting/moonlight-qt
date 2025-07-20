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
    property int unitIndex: 0 // 0=小时 1=天 2=周 3=月
    property int quantity: 1

    title: ""
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
        case 0: unitPrice = timingPrice; break
        case 1: unitPrice = dayPrice; break
        case 2: unitPrice = weekPrice; break
        case 3: unitPrice = monthPrice; break
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
        spacing: 16

        Label {
            text: qsTr("请选择续费方式")
            font.pointSize: 14
            font.bold: true
            Layout.alignment: Qt.AlignHCenter
        }

        Label {
            textFormat: Text.RichText
            text: qsTr("共计 ") + "<font color='#FFBF00' style='font-size:16pt; font-weight:bold;'>" + totalPrice + "</font>" + qsTr(" 金币")
            font.pointSize: 12
            horizontalAlignment: Text.AlignHCenter
            Layout.alignment: Qt.AlignHCenter
        }

        // 单选按钮：小时、天、周、月
        RowLayout {
            Layout.alignment: Qt.AlignHCenter
            spacing: 20
            Repeater {
                model: [qsTr("小时"), qsTr("天"), qsTr("周"), qsTr("月")]
                delegate: RadioButton {
                    text: modelData
                    checked: index === unitIndex
                    onClicked: unitIndex = index
                }
            }
        }

        // 数量选择
        RowLayout {
            Layout.alignment: Qt.AlignHCenter
            spacing: 8
            Button {
                text: "-"
                enabled: quantity > 1
                onClicked: quantity--
            }
            Label {
                text: quantity
                font.pointSize: 12
            }
            Button {
                text: "+"
                onClicked: quantity++
            }
        }
    }

    onAccepted: {
        var typeMap = [7, 2, 3, 4]  // 与 unitIndex 映射顺序一致：小时、天、周、月
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
