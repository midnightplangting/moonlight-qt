import QtQuick 2.9
import QtQuick.Controls 2.2
import QtQuick.Layouts 1.3
import QtQuick.Window 2.2
import QtQuick.Controls.Material 2.2

import ComputerManager 1.0
import AutoUpdateChecker 1.0
import StreamingPreferences 1.0
import SystemProperties 1.0
import SdlGamepadKeyNavigation 1.0

ApplicationWindow {
    flags: Qt.FramelessWindowHint | Qt.Window | Qt.WindowMinimizeButtonHint

    property bool pollingActive: false

    // Set by SettingsView to force the back operation to pop all
    // pages except the initial view. This is required when doing
    // a retranslate() because AppView breaks for some reason.
    property bool clearOnBack: false

    id: window
    width: 1280
    height: 800
    // 设置窗口的背景为透明，确保圆角效果可见
    color: "transparent"  // 设置背景颜色为透明

    // 设置窗口的背景并使底部有圆角
    Rectangle {
        width: parent.width
        height: parent.height
        radius: 20  // 设置底部圆角的半径
        color: "#000"  // 设置窗口背景颜色
        anchors.fill: parent
    }
    // 如果需要让窗口支持透明的背景，则可以将窗口背景透明
    opacity: 1  // 确保不透明度为 1，避免透明效果影响布局



    // This function runs prior to creation of the initial StackView item
    function doEarlyInit() {
        // Override the background color to Material 2 colors for Qt 6.5+
        // in order to improve contrast between GFE's placeholder box art
        // and the background of the app grid.
        if (SystemProperties.usesMaterial3Theme) {
            Material.background = "#000"
        }

        SdlGamepadKeyNavigation.enable()
    }

    Component.onCompleted: {
        // Show the window according to the user's preferences
        if (SystemProperties.hasDesktopEnvironment) {
            if (StreamingPreferences.uiDisplayMode == StreamingPreferences.UI_MAXIMIZED) {
                window.showMaximized()
            }
            else if (StreamingPreferences.uiDisplayMode == StreamingPreferences.UI_FULLSCREEN) {
                window.showFullScreen()
            }
            else {
                window.show()
            }
        } else {
            window.showFullScreen()
        }

        // Display any modal dialogs for configuration warnings
        if (SystemProperties.isWow64) {
            wow64Dialog.open()
        }
        else if (!SystemProperties.hasHardwareAcceleration && StreamingPreferences.videoDecoderSelection !== StreamingPreferences.VDS_FORCE_SOFTWARE) {
            if (SystemProperties.isRunningXWayland) {
                xWaylandDialog.open()
            }
            else {
                noHwDecoderDialog.open()
            }
        }

        if (SystemProperties.unmappedGamepads) {
            unmappedGamepadDialog.unmappedGamepads = SystemProperties.unmappedGamepads
            unmappedGamepadDialog.open()
        }
    }
  
    // It would be better to use TextMetrics here, but it always lays out
    // the text slightly more compactly than real Text does in ToolTip,
    // causing unexpected line breaks to be inserted
    Text {
        id: tooltipTextLayoutHelper
        visible: false
        font: ToolTip.toolTip.font
        text: ToolTip.toolTip.text
    }

    // This configures the maximum width of the singleton attached QML ToolTip. If left unconstrained,
    // it will never insert a line break and just extend on forever.
    ToolTip.toolTip.contentWidth: Math.min(tooltipTextLayoutHelper.width, 400)

    function goBack() {
        if (clearOnBack) {
            // Pop all items except the first one
            stackView.pop(null)
            clearOnBack = false
        }
        else {
            stackView.pop()
        }
    }

    StackView {
        id: stackView
        anchors.fill: parent
        focus: true

        Component.onCompleted: {
            // Perform our early initialization before constructing
            // the initial view and pushing it to the StackView
            doEarlyInit()
            push(initialView)
        }

        onCurrentItemChanged: {
            // Ensure focus travels to the next view when going back
            if (currentItem) {
                currentItem.forceActiveFocus()
            }
        }

        Keys.onEscapePressed: {
            if (depth > 1) {
                goBack()
            }
            else {
                quitConfirmationDialog.open()
            }
        }

        Keys.onBackPressed: {
            if (depth > 1) {
                goBack()
            }
            else {
                quitConfirmationDialog.open()
            }
        }

        Keys.onMenuPressed: {
            settingsButton.clicked()
        }

        // This is a keypress we've reserved for letting the
        // SdlGamepadKeyNavigation object tell us to show settings
        // when Menu is consumed by a focused control.
        Keys.onHangupPressed: {
            settingsButton.clicked()
        }
    }

    // This timer keeps us polling for 5 minutes of inactivity
    // to allow the user to work with Moonlight on a second display
    // while dealing with configuration issues. This will ensure
    // machines come online even if the input focus isn't on Moonlight.
    Timer {
        id: inactivityTimer
        interval: 5 * 60000
        onTriggered: {
            if (!active && pollingActive) {
                ComputerManager.stopPollingAsync()
                pollingActive = false
            }
        }
    }

    onVisibleChanged: {
        // When we become invisible while streaming is going on,
        // stop polling immediately.
        if (!visible) {
            inactivityTimer.stop()

            if (pollingActive) {
                ComputerManager.stopPollingAsync()
                pollingActive = false
            }
        }
        else if (active) {
            // When we become visible and active again, start polling
            inactivityTimer.stop()

            // Restart polling if it was stopped
            if (!pollingActive) {
                ComputerManager.startPolling()
                pollingActive = true
            }
        }

        // Poll for gamepad input only when the window is in focus
        SdlGamepadKeyNavigation.notifyWindowFocus(visible && active)
    }

    onActiveChanged: {
        if (active) {
            // Stop the inactivity timer
            inactivityTimer.stop()

            // Restart polling if it was stopped
            if (!pollingActive) {
                ComputerManager.startPolling()
                pollingActive = true
            }
        }
        else {
            // Start the inactivity timer to stop polling
            // if focus does not return within a few minutes.
            inactivityTimer.restart()
        }

        // Poll for gamepad input only when the window is in focus
        SdlGamepadKeyNavigation.notifyWindowFocus(visible && active)
    }

    // Workaround for lack of instanceof in Qt 5.9.
    //
    // Based on https://stackoverflow.com/questions/13923794/how-to-do-a-is-a-typeof-or-instanceof-in-qml
    function qmltypeof(obj, className) { // QtObject, string -> bool
        // className plus "(" is the class instance without modification
        // className plus "_QML" is the class instance with user-defined properties
        var str = obj.toString();
        return str.startsWith(className + "(") || str.startsWith(className + "_QML");
    }

    function navigateTo(url, objectType)
    {
        var existingItem = stackView.find(function(item, index) {
            return qmltypeof(item, objectType)
        })

        if (existingItem !== null) {
            // Pop to the existing item
            stackView.pop(existingItem)
        }
        else {
            // Create a new item
            stackView.push(url)
        }
    }
    ToolBar {
        id: toolBar  // 设置 ToolBar 的 ID，用于引用该控件
        height: 60  // 设置 ToolBar 的高度为 60 像素
        anchors.top: parent.top
        anchors.left: parent.left
        anchors.right: parent.right
        background: Rectangle {
            color: "#333"  // ToolBar 的背景颜色
            radius: 20  // 圆角半径
        }
        // 鼠标区域用于拖动窗口
        MouseArea {
            id: dragArea  // 设置 MouseArea 的 ID
            anchors.fill: parent  // 使 MouseArea 填充父元素（ToolBar）
            drag.target: null  // 禁用拖动控件的功能，确保 ToolBar 仅为拖动窗口
            onPressed: {
                window.startSystemMove()  // 按下鼠标时启动系统窗口移动
            }
        }

        // 顶部的标题标签
        Label {
            id: titleLabel  // 设置 Label 的 ID
            visible: toolBar.width > 700  // 当 ToolBar 宽度大于 700 像素时，标题显示
            anchors.fill: parent  // 使标题标签填充整个父元素（ToolBar）
            text: stackView.currentItem.objectName  // 显示当前堆栈视图中项目的名称
            font.pointSize: 20  // 设置字体大小为 20
            elide: Label.ElideRight  // 在文本过长时，从右侧截断并显示省略号
            horizontalAlignment: Qt.AlignHCenter  // 设置文本的水平对齐方式为居中
            verticalAlignment: Qt.AlignVCenter  // 设置文本的垂直对齐方式为居中
        }

        ColumnLayout {
            anchors.fill: parent  // 使 ColumnLayout 填充整个父元素
            spacing: 8  // 设置 ColumnLayout 内部项的间距为 4 像素

            // RowLayout 用于布局横向的控件（例如 logo、返回按钮、标题等）
            RowLayout {
                spacing: 5  // 设置 RowLayout 内部控件的间距为 10 像素
                anchors.leftMargin: 5  // 设置 RowLayout 左侧的外边距为 10 像素
                anchors.rightMargin: 5  // 设置 RowLayout 右侧的外边距为 10 像素
                anchors.fill: parent  // 使 RowLayout 填充整个父元素
                Layout.fillWidth: true  // 使 RowLayout 填充可用的宽度

                // 显示的 Logo 图标
                Image {
                    source: "qrc:/res/discord.svg"  // 设置 logo 图标的路径
                    width: 10  // 设置图标的宽度为 10 像素
                    height: 10  // 设置图标的高度为 10 像素
                    fillMode: Image.PreserveAspectFit  // 保持图像的宽高比
                    anchors.verticalCenter: parent.verticalCenter  // 垂直居中对齐
                }

                // 返回按钮
                NavigableToolButton {
                    visible: stackView.depth > 1  // 当堆栈深度大于 1 时显示返回按钮
                    iconSource: "qrc:/res/arrow_left.svg"  // 设置返回按钮的图标
                    onClicked: goBack()  // 点击时触发返回操作
                    Keys.onDownPressed: { stackView.currentItem.forceActiveFocus(Qt.TabFocus) }  // 按下 Tab 键时切换焦点
                }

                // 当窗口太小而需要确保控件不发生重叠时，使用 Label 来占位
                Label {
                    id: titleRowLabel  // 设置 Label 的 ID
                    font.pointSize: titleLabel.font.pointSize  // 设置字体大小与标题一致
                    elide: Label.ElideRight  // 长文本会被截断
                    horizontalAlignment: Qt.AlignHCenter  // 水平居中对齐
                    verticalAlignment: Qt.AlignVCenter  // 垂直居中对齐
                    Layout.fillWidth: true  // 填充 RowLayout 的剩余空间
                    text: !titleLabel.visible ? stackView.currentItem.objectName : ""  // 如果标题不可见，显示当前项目名称，否则为空
                }

                // 版本号标签，只有在 SettingsView 中才可见
                Label {
                    id: versionLabel  // 设置 Label 的 ID
                    visible: qmltypeof(stackView.currentItem, "SettingsView")  // 只有在 SettingsView 中可见
                    text: qsTr("Version %1").arg(SystemProperties.versionString)  // 显示版本号
                    font.pointSize: 12  // 设置字体大小为 12
                    horizontalAlignment: Qt.AlignRight  // 右对齐
                    verticalAlignment: Qt.AlignVCenter  // 垂直居中对齐
                }

                // 最小化按钮
                NavigableToolButton {
                    iconSource: "qrc:/res/minimize.svg"  // 设置图标路径
                    ToolTip.text: qsTr("最小化")  // 设置工具提示文本
                    onClicked: window.showMinimized()  // 点击时最小化窗口
                }

                // 最大化/还原按钮
                NavigableToolButton {
                    iconSource: "qrc:/res/fullscreen.svg"  // 设置图标路径
                    ToolTip.text: qsTr("全屏/还原")  // 设置工具提示文本
                    onClicked: {
                        if (window.visibility === Window.Maximized || window.visibility === Window.FullScreen) {
                            window.showNormal()  // 如果窗口已最大化或全屏，则恢复正常大小
                        } else {
                            window.showMaximized()  // 否则最大化窗口
                        }
                    }
                }

                // 关闭按钮
                NavigableToolButton {
                    iconSource: "qrc:/res/close.svg"  // 设置图标路径
                    ToolTip.text: qsTr("关闭")  // 设置工具提示文本
                    onClicked: Qt.quit()  // 点击时退出应用
                }
            }


            RowLayout {
                Layout.fillWidth: true
                Layout.preferredHeight: 30
                spacing: 8

                // 📢 左边：喇叭图标 + “公示：”
                RowLayout {
                    spacing: 4
                    Layout.preferredWidth: 100  // 可根据内容调整
                    Layout.alignment: Qt.AlignVCenter

                    Image {
                        source: "qrc:/res/update.svg"  // 你可以换成 emoji 或 SVG 图标
                        width: 16
                        height: 16
                        fillMode: Image.PreserveAspectFit
                    }

                    Label {
                        text: qsTr("公示：")
                        color: "#fff"
                        font.pointSize: 12
                    }
                }

                Rectangle {
                    id: marqueeBox
                    Layout.fillWidth: true
                    height: 30
                    radius: 4
                    color: "#333"
                    clip: true

                    property int scrollSpeed: 1            // 每帧移动像素数
                    property int scrollInterval: 16        // 刷新频率（毫秒）

                    Text {
                        id: marqueeText
                        text: "📢 欢迎使用 Moonlight！请点击右上角的设置按钮体验更多功能～"
                        font.pointSize: 12
                        y: 4
                        x: marqueeBox.width
                        color: "#fff"
                    }

                    Timer {
                        id: scrollTimer
                        interval: marqueeBox.scrollInterval
                        repeat: true
                        running: true
                        onTriggered: {
                            marqueeText.x -= marqueeBox.scrollSpeed
                            if (marqueeText.x + marqueeText.width < 0) {
                                marqueeText.x = marqueeBox.width
                            }
                        }
                    }

                    MouseArea {
                        anchors.fill: parent
                        hoverEnabled: true
                        onEntered: scrollTimer.stop()
                        onExited: scrollTimer.start()
                    }
                }

                // ⚙️ 右边：设置按钮
                NavigableToolButton {
                    id: settingsButton1

                    iconSource:  "qrc:/res/settings.svg"

                    onClicked: navigateTo("qrc:/gui/SettingsView.qml", "SettingsView")

                    Keys.onDownPressed: {
                        stackView.currentItem.forceActiveFocus(Qt.TabFocus)
                    }

                    Shortcut {
                        id: settingsShortcut1
                        sequence: StandardKey.Preferences
                        onActivated: settingsButton.clicked()
                    }

                    ToolTip.delay: 1000
                    ToolTip.timeout: 3000
                    ToolTip.visible: hovered
                    ToolTip.text: qsTr("Settings") + (settingsShortcut.nativeText ? (" ("+settingsShortcut.nativeText+")") : "")
                }
            }
    }

    ErrorMessageDialog {
        id: noHwDecoderDialog
        text: qsTr("No functioning hardware accelerated video decoder was detected by Moonlight. " +
                   "Your streaming performance may be severely degraded in this configuration.")
        helpText: qsTr("Click the Help button for more information on solving this problem.")
        helpUrl: "https://github.com/moonlight-stream/moonlight-docs/wiki/Fixing-Hardware-Decoding-Problems"
    }

    ErrorMessageDialog {
        id: xWaylandDialog
        text: qsTr("Hardware acceleration doesn't work on XWayland. Continuing on XWayland may result in poor streaming performance. " +
                   "Try running with QT_QPA_PLATFORM=wayland or switch to X11.")
        helpText: qsTr("Click the Help button for more information.")
        helpUrl: "https://github.com/moonlight-stream/moonlight-docs/wiki/Fixing-Hardware-Decoding-Problems"
    }

    NavigableMessageDialog {
        id: wow64Dialog
        standardButtons: Dialog.Ok | Dialog.Cancel
        text: qsTr("This version of Moonlight isn't optimized for your PC. Please download the '%1' version of Moonlight for the best streaming performance.").arg(SystemProperties.friendlyNativeArchName)
        onAccepted: {
            Qt.openUrlExternally("https://github.com/moonlight-stream/moonlight-qt/releases");
        }
    }

    ErrorMessageDialog {
        id: unmappedGamepadDialog
        property string unmappedGamepads : ""
        text: qsTr("Moonlight detected gamepads without a mapping:") + "\n" + unmappedGamepads
        helpTextSeparator: "\n\n"
        helpText: qsTr("Click the Help button for information on how to map your gamepads.")
        helpUrl: "https://github.com/moonlight-stream/moonlight-docs/wiki/Gamepad-Mapping"
    }

    // This dialog appears when quitting via keyboard or gamepad button
    NavigableMessageDialog {
        id: quitConfirmationDialog
        standardButtons: Dialog.Yes | Dialog.No
        text: qsTr("Are you sure you want to quit?")
        // For keyboard/gamepad navigation
        onAccepted: Qt.quit()
    }

    // HACK: This belongs in StreamSegue but keeping a dialog around after the parent
    // dies can trigger bugs in Qt 5.12 that cause the app to crash. For now, we will
    // host this dialog in a QML component that is never destroyed.
    //
    // To repro: Start a stream, cut the network connection to trigger the "Connection
    // terminated" dialog, wait until the app grid times out back to the PC grid, then
    // try to dismiss the dialog.
    ErrorMessageDialog {
        id: streamSegueErrorDialog

        property bool quitAfter: false

        onClosed: {
            if (quitAfter) {
                Qt.quit()
            }

            // StreamSegue assumes its dialog will be re-created each time we
            // start streaming, so fake it by wiping out the text each time.
            text = ""
        }
    }

    NavigableDialog {
        id: addPcDialog
        property string label: qsTr("Enter the IP address of your host PC:")

        standardButtons: Dialog.Ok | Dialog.Cancel

        onOpened: {
            // Force keyboard focus on the textbox so keyboard navigation works
            editText.forceActiveFocus()
        }

        onClosed: {
            editText.clear()
        }

        onAccepted: {
            if (editText.text) {
                ComputerManager.addNewHostManually(editText.text.trim())
            }
        }

        ColumnLayout {
            Label {
                text: addPcDialog.label
                font.bold: true
            }

            TextField {
                id: editText
                Layout.fillWidth: true
                focus: true

                Keys.onReturnPressed: {
                    addPcDialog.accept()
                }

                Keys.onEnterPressed: {
                    addPcDialog.accept()
                }
            }
        }
    }
}
}
