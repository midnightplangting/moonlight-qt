import QtQuick 2.9
import QtQuick.Controls 2.2
import QtQuick.Layouts 1.3

Page {
    id: root
    title: qsTr("GPU 购买")

    property int currentTab: 0  // 0=计时,1=包机

    StackView.onActivated: {
        gpuModel.refresh()
    }

    /* 滑块切换 */
    Rectangle {
        id: modeSwitch
        anchors.top: parent.top; anchors.horizontalCenter: parent.horizontalCenter; anchors.topMargin: 16
        width: 400; height: 32; radius: 5; color: "#1C1C1E"

        Rectangle {
            id: thumb; width: modeSwitch.width/2; height: modeSwitch.height; radius: 10; color: "#3A3A3C"
        }
        states: [
            State { name: "left"; when: currentTab===0; PropertyChanges{target:thumb; x:0} },
            State { name: "right"; when: currentTab===1; PropertyChanges{target:thumb; x: modeSwitch.width/2} }
        ]
        transitions: [ Transition { from:"*"; to:"*"; NumberAnimation{properties:"x"; duration:200}} ]

        Row { anchors.fill: parent
            Repeater {
                model: [qsTr("计时"), qsTr("包机")]
                delegate: Item {
                    width: modeSwitch.width/2; height: modeSwitch.height
                    Text { anchors.centerIn: parent; text: modelData; font.pixelSize:16;
                           color: index===currentTab?"white":"#CCCCCC" }
                    MouseArea { anchors.fill: parent; onClicked: currentTab=index }
                }
            }
        }
    }

    /* 卡片列表 */
    Flickable {
        id: flick
        anchors.top: modeSwitch.bottom; anchors.topMargin:30; anchors.horizontalCenter: parent.horizontalCenter
        width: Math.min(parent.width, 4*240 + 3*24 + 48)
        height: parent.height - modeSwitch.height - 130
        flickableDirection: Flickable.VerticalFlick
        contentWidth: width; contentHeight: cardGrid.height

        Item { width: flick.width; height: cardGrid.height
            Grid {
                id: cardGrid
                anchors.top: parent.top; anchors.horizontalCenter: parent.horizontalCenter
                columns:4; columnSpacing:24; rowSpacing:24

                Repeater {
                    model: gpuModel
                    delegate: Rectangle {
                        id: card
                        width:240; height: currentTab===0?200:235; radius:12;
                        // ① 对 height 加动画
                        Behavior on height {
                            NumberAnimation {
                                duration: 200
                                easing.type: Easing.OutQuad
                            }
                        }
                        color: hovered?"#4A4A4A":"#1C1C1E"

                        // 缩放效果
                        scale: 1
                        Behavior on scale { NumberAnimation { duration: 100; easing.type: Easing.OutQuad } }

                        property bool hovered: false
                        property int packageMode: 0
                        property int currentPrice: currentTab===0? hourly : (packageMode===0? day : (packageMode===1? week : month))

                        MouseArea {
                            anchors.fill: parent; hoverEnabled: true
                            onEntered: {hovered = true; card.scale = 1.02}
                            onExited: {hovered = false; card.scale = 1.0}
                            onPressed:  card.scale = 0.95
                            onReleased: card.scale = 1.02
                            // 如果有卡片点击逻辑，可在这里处理
                        }

                        Column {
                            anchors.fill: parent; anchors.margins:16; spacing:14
                            Text { text: name; font.pixelSize:22; color:"white"; horizontalAlignment:Text.AlignHCenter; width:parent.width }
                            Text { text: qsTr("最高视频码率：") + bitrate + " Mbps"; font.pixelSize:12; color:"#CCCCCC"; horizontalAlignment:Text.AlignHCenter; width:parent.width }

                            // 包机模式切换
                            Rectangle {
                                id: pkgSwitch
                                visible: currentTab===1
                                width: parent.width; height:24; radius:12; color:"#1C1C14"
                                anchors.horizontalCenter: parent.horizontalCenter

                                Rectangle {
                                    id: pkgThumb
                                    width: pkgSwitch.width/3; height: pkgSwitch.height; radius:12; color:"#3A3A3C"
                                    x: packageMode * pkgSwitch.width/3
                                    Behavior on x { NumberAnimation { duration:200 } }
                                }
                                Row { anchors.fill: parent
                                    Repeater {
                                        model: [qsTr("包天"), qsTr("包周"), qsTr("包月")]
                                        delegate: Item {
                                            width: pkgSwitch.width/3; height: pkgSwitch.height
                                            Text { anchors.centerIn: parent; text:modelData; font.pixelSize:12;
                                                   color: index===packageMode?"white":"#DDDDDD" }
                                            MouseArea { anchors.fill: parent; onClicked: packageMode=index }
                                        }
                                    }
                                }
                            }

                            Row { anchors.horizontalCenter:parent.horizontalCenter; spacing:4
                                Text { text: qsTr("当前价格"); font.pixelSize:15; color:"#CCCCCC" }
                                Text { text: currentPrice + qsTr(" 金币") + (currentTab===0?qsTr("/小时"):""); font.pixelSize:18; font.bold:true; color:"#FFA500" }
                            }
                            Row { anchors.horizontalCenter:parent.horizontalCenter; spacing:4
                                Text { text: qsTr("剩余设备数量"); font.pixelSize:12; color:"#CCCCCC" }
                                Text { text: deviceCount + qsTr(" 台"); font.pixelSize:15; font.bold:true; color:"white" }
                            }

                            // 开机按钮
                            Button {
                                id: startBtn
                                text: qsTr("开机"); width:60; height:36; font.pixelSize:16; anchors.horizontalCenter:parent.horizontalCenter

                                // 缩放效果
                                scale: 1
                                Behavior on scale { NumberAnimation { duration: 80; easing.type: Easing.OutQuad } }

                                onPressed:  startBtn.scale = 0.9
                                onReleased: startBtn.scale = 1

                                background: Rectangle{anchors.fill:parent;color:"transparent"}
                                contentItem: Text{anchors.centerIn:parent;text:startBtn.text;font.pixelSize:16;color:"#2196F3"}
                                onClicked: console.log("Start", name, currentPrice)
                            }
                        }
                    }
                }
            }
        }
    }

    /* 内嵌手动添加 */
    Rectangle {
        anchors.top: flick.bottom; anchors.topMargin:24; anchors.horizontalCenter:parent.horizontalCenter
        width: flick.width - 400; height:48; radius:24; color:"#1C1C1E"
        Row{anchors.fill:parent;anchors.margins:12;spacing:8
            Image{source:"qrc:/res/ic_add_to_queue_white_48px.svg";width:24;height:24}
            TextField{placeholderText:qsTr("手动添加电脑                                                                                                                                                                        ");
                font.pixelSize:14;color:"#DDDDDD";background:Rectangle{color:"transparent"}
                onAccepted:{console.log("Add PC:", text); text=""}
            }
        }
    }
}
