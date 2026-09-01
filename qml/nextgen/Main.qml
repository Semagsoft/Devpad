import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Dialogs
import Devpad.Nextgen 1.0

ApplicationWindow {
    id: win
    visible: true
    width: 1400
    height: 900
    title: "Devpad Nextgen — primoEditor " + devpadVersion
    color: palette.window

    property var openFiles: initialFiles
    property string folder: initialFolder
    property var actions: nextgenActions

    Component.onCompleted: console.log("NEXTGEN_MAIN_LOADED with menuBar=" + (menuBar ? "yes" : "no") + " headerVisible=" + (header ? "yes" : "no"))

    // MenuBar native on macOS via Qt.labs.platform would be applied here if needed;
    // Controls MenuBar works everywhere and is native-ish on macOS as well.
    menuBar: NextgenMenuBar {
        actions: win.actions
        editor: primo
    }

    header: ColumnLayout {
        spacing: 0
        NextgenToolBar {
            Layout.fillWidth: true
            actions: win.actions
            editor: primo
        }
        // Large file read-only banner
        Pane {
            id: largeBanner
            Layout.fillWidth: true
            visible: primo.readOnly && primo.document && primo.document.length > 50*1024*1024
            padding: 6
            background: Rectangle { color: "#ffcc00"; opacity: 0.9 }
            RowLayout {
                anchors.fill: parent
                Label { text: "⚠ Large file >50MB — read-only, async highlight only (visible + 200 lines)"; color: "black"; font.pixelSize: 11; Layout.fillWidth: true }
                ToolButton { text: "Force Edit"; onClicked: primo.readOnly = false }
            }
        }
    }

    footer: NextgenStatusBar {
        actions: win.actions
        editor: primo
    }

    SplitView {
        anchors.fill: parent
        orientation: Qt.Horizontal

        Pane {
            SplitView.preferredWidth: 260
            SplitView.minimumWidth: 180
            padding: 6
            ColumnLayout {
                anchors.fill: parent
                spacing: 6
                Label { text: folder ? folder : "No folder"; Layout.fillWidth: true; opacity: 0.8; font.pixelSize: 12; wrapMode: Text.NoWrap }
                ScrollView {
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    clip: true
                    ListView {
                        id: fileList
                        model: openFiles
                        delegate: ItemDelegate {
                            width: fileList.width
                            text: modelData
                            onClicked: primo.loadFile(modelData)
                        }
                    }
                }
                Label { text: "Perf: QSG TextNode + visible cull + async highlight"; font.pixelSize: 10; opacity: 0.5; wrapMode: Text.Wrap }
            }
        }

        ColumnLayout {
            SplitView.fillWidth: true
            spacing: 0

            TabBar {
                id: tabBar
                Layout.fillWidth: true
                visible: actions ? actions.showToolbar : true // placeholder, could use tabBar setting
                Repeater {
                    model: openFiles.length ? openFiles : ["Untitled"]
                    TabButton { text: modelData.split("/").pop(); width: implicitWidth + 24; onClicked: primo.loadFile(modelData) }
                }
            }

            // High-perf QSG editor + minimap + gutter
            RowLayout {
                Layout.fillWidth: true
                Layout.fillHeight: true
                spacing: 0

                Flickable {
                    id: flick
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    clip: true
                    contentHeight: primo.contentHeight
                    contentWidth: width
                    boundsBehavior: Flickable.StopAtBounds
                    ScrollBar.vertical: ScrollBar { policy: ScrollBar.AlwaysOn }
                    ScrollBar.horizontal: ScrollBar { policy: ScrollBar.AsNeeded }

                    PrimoEditor {
                        id: primo
                        width: flick.width
                        height: contentHeight
                        focus: true
                        gutterVisible: true
                        relativeNumbers: false
                        backgroundColor: palette.base
                        foregroundColor: palette.text
                        font.family: "Monospace"
                        font.pixelSize: 13
                        Component.onCompleted: {
                            if (nextgenActions) nextgenActions.editor = primo;
                            forceActiveFocus();
                            if (openFiles.length) loadFile(openFiles[0])
                        }
                        onFileLoaded: (ok, error) => {
                            if (!ok) console.warn("load failed:", error)
                            else flick.contentY = 0
                        }
                        onDiagnosticsChanged: minimap.requestPaint()
                        onBookmarksChanged: minimap.requestPaint()
                        onTextChanged: minimap.requestPaint()
                    }
                }

                Minimap {
                    id: minimap
                    visible: primo.minimapVisible
                    Layout.preferredWidth: 100
                    Layout.fillHeight: true
                    editor: primo
                    flick: flick
                    bg: palette.base
                }
            }
            RowLayout {
                Layout.fillWidth: true
                spacing: 8
                CheckBox { text: "Gutter"; checked: primo.gutterVisible; onToggled: primo.gutterVisible = checked }
                CheckBox { text: "Rel. numbers"; checked: primo.relativeNumbers; onToggled: primo.relativeNumbers = checked }
                CheckBox { text: "Minimap"; checked: primo.minimapVisible; onToggled: primo.minimapVisible = checked }
                CheckBox { text: "Read-only"; checked: primo.readOnly; onToggled: primo.readOnly = checked }
                Label { text: "F2/Ctrl+B toggle bookmark • gutter click"; opacity: 0.5; font.pixelSize: 10 }
                Item { Layout.fillWidth: true }
            }
        }
    }

    // Dialogs — Qt6 QtQuick.Dialogs
    FileDialog {
        id: openFileDlg
        title: "Open File"
        fileMode: FileDialog.OpenFile
        nameFilters: ["All Files (*)", "C++ (*.cpp *.h *.hpp)", "Python (*.py)", "QML (*.qml)", "JavaScript (*.js)", "Text (*.txt)"]
        onAccepted: {
            var path = selectedFile.toString().replace("file://", "");
            path = decodeURIComponent(path);
            actions.openFile(path);
            openFiles = actions.recentFiles;
        }
    }
    FolderDialog {
        id: openFolderDlg
        title: "Open Folder"
        currentFolder: folder ? "file://" + folder : ""
        onAccepted: {
            var path = selectedFolder.toString().replace("file://", "");
            path = decodeURIComponent(path);
            folder = path;
            actions.openFile(path);
        }
    }
    FileDialog {
        id: saveAsDlg
        title: "Save As"
        fileMode: FileDialog.SaveFile
        nameFilters: ["All Files (*)"]
        onAccepted: {
            var path = selectedFile.toString().replace("file://", "");
            path = decodeURIComponent(path);
            primo.saveAs(path);
        }
    }

    // Go to line dialog
    Dialog {
        id: gotoDlg
        title: "Go to Line"
        modal: true
        standardButtons: Dialog.Ok | Dialog.Cancel
        property int lineNum: 1
        ColumnLayout {
            anchors.fill: parent
            Label { text: "Line number:" }
            SpinBox {
                id: gotoSpin
                from: 1; to: primo.document ? primo.document.lineCount : 999999
                value: primo.cursorLine + 1
                editable: true
            }
        }
        onAccepted: {
            primo.setCursorPosition(gotoSpin.value - 1, 0);
            primo.forceActiveFocus();
            // scroll to
            flick.contentY = Math.max(0, (gotoSpin.value - 1) * primo.lineHeight - flick.height/2);
        }
    }

    // Find placeholder dialog
    Dialog {
        id: findDlg
        title: "Find"
        modal: false
        standardButtons: Dialog.Close
        ColumnLayout {
            anchors.fill: parent
            Label { text: "Find (placeholder – use Ctrl+F in editor)" }
            TextField { id: findField; placeholderText: "Search..."; onAccepted: console.log("find:", text) }
        }
    }

    // About
    Dialog {
        id: aboutDlg
        title: "About Devpad Nextgen"
        modal: true
        standardButtons: Dialog.Ok
        Label {
            text: "Devpad Nextgen — primoEditor " + devpadVersion + "\nQSG high-perf, gutter + minimap + async highlight\n\nSemagsoft 2026"
            wrapMode: Text.Wrap
        }
    }

    // Options placeholder
    Dialog {
        id: optionsDlg
        title: "Options"
        modal: true
        standardButtons: Dialog.Close
        Label { text: "Options: use SettingsManager via toolbar/menu persistence already shared." }
    }

    Connections {
        target: actions
        function onRequestOpenFileDialog() { openFileDlg.open(); }
        function onRequestOpenFolderDialog() { openFolderDlg.open(); }
        function onRequestSaveAsDialog() { saveAsDlg.open(); }
        function onRequestGoToLineDialog() { gotoDlg.open(); }
        function onRequestFindDialog() { findDlg.open(); }
        function onRequestReplaceDialog() { findDlg.open(); }
        function onRequestAboutDialog() { aboutDlg.open(); }
        function onRequestOptionsDialog() { optionsDlg.open(); }
        function onShowMessage(msg) {
            statusMsg.text = msg;
            statusMsgTimer.restart();
        }
    }

    // transient message overlay (above statusbar)
    Label {
        id: statusMsg
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.bottom: parent.bottom
        anchors.bottomMargin: 28
        visible: text !== ""
        padding: 8
        background: Rectangle { color: "#333"; radius: 4; opacity: 0.9 }
        color: "white"
        font.pixelSize: 12
        Timer { id: statusMsgTimer; interval: 3000; onTriggered: statusMsg.text = "" }
        Behavior on opacity { NumberAnimation { duration: 200 } }
    }

    Shortcut { sequence: "Ctrl+S"; onActivated: actions.saveFile() }
    Shortcut { sequence: "Ctrl+Shift+S"; onActivated: actions.saveFileAsDialog() }
    Shortcut { sequence: "Ctrl+O"; onActivated: actions.openFileDialog() }
    Shortcut { sequence: "Ctrl+G"; onActivated: actions.goToLine() }
    Shortcut { sequence: "Ctrl+F"; onActivated: actions.find() }
    Shortcut { sequence: "Ctrl+H"; onActivated: actions.replace() }
    Shortcut { sequence: "Ctrl+B"; onActivated: actions.toggleBookmark() }
    Shortcut { sequence: "F2"; onActivated: actions.toggleBookmark() }
}
