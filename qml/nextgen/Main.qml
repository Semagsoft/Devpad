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
    property var tabs: tabModel

    Component.onCompleted: {
        console.log("NEXTGEN_MAIN_LOADED with menuBar=" + (menuBar ? "yes" : "no"))
        // Defer FindBar editor binding until editors are created
        findBar.editor = primo
        findBar.flick = flick
    }

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
        FindBar {
            id: findBar
            Layout.fillWidth: true
        }
        Pane {
            id: largeBanner
            Layout.fillWidth: true
            visible: primo.readOnly && primo.document && primo.document.length > 50*1024*1024
            padding: 6
            background: Rectangle { color: "#ffcc00"; opacity: 0.9 }
            RowLayout {
                anchors.fill: parent
                Label { text: "⚠ Large file >50MB — read-only, incremental undo disabled, async highlight only"; color: "black"; font.pixelSize: 11; Layout.fillWidth: true }
                ToolButton { text: "Force Edit"; onClicked: { primo.readOnly = false; if (primo2) primo2.readOnly = false } }
            }
        }
    }

    footer: NextgenStatusBar {
        actions: win.actions
        editor: primo.activeFocus ? primo : (primo2 && primo2.activeFocus ? primo2 : primo)
    }

    // For split detection
    property bool splitSplit: tabs ? tabs.splitVisible : false

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
                            onClicked: {
                                tabs.addTab(modelData)
                                primo.loadFile(modelData)
                                if (tabs.splitVisible && primo2) primo2.loadFile(modelData)
                            }
                        }
                    }
                }
                RowLayout {
                    Layout.fillWidth: true
                    ToolButton {
                        text: tabs && tabs.splitVisible ? "Unsplit" : "Split"
                        onClicked: tabs.splitVisible = !tabs.splitVisible
                    }
                    Label { text: tabs ? tabs.count + " tabs" : "0 tabs"; opacity: 0.6; font.pixelSize: 10; Layout.fillWidth: true }
                }
                Label { text: "Perf: QSG + undo disabled >50MB"; font.pixelSize: 10; opacity: 0.5; wrapMode: Text.Wrap }
            }
        }

        ColumnLayout {
            SplitView.fillWidth: true
            spacing: 0

            // Primary pane TabBar draggable
            DraggableTabBar {
                id: tabBar1
                Layout.fillWidth: true
                tabModel: tabs
                paneId: 0
                editor: primo
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
                            if (tabs && tabs.count >0) {
                                var f = tabs.currentFile()
                                if (f) loadFile(f)
                                else if (openFiles.length) loadFile(openFiles[0])
                            } else if (openFiles.length) loadFile(openFiles[0])
                        }
                        onFileLoaded: (ok, error) => {
                            if (!ok) console.warn("load failed:", error)
                            else flick.contentY = 0
                        }
                        onDiagnosticsChanged: minimap.requestPaint()
                        onBookmarksChanged: minimap.requestPaint()
                        onTextChanged: minimap.requestPaint()
                        // Theme change handled via NextgenActions
                    }
                }

                // Second pane for split
                Flickable {
                    id: flick2
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    clip: true
                    visible: tabs ? tabs.splitVisible : false
                    contentHeight: primo2.contentHeight
                    contentWidth: width
                    boundsBehavior: Flickable.StopAtBounds
                    ScrollBar.vertical: ScrollBar { policy: ScrollBar.AlwaysOn }
                    ScrollBar.horizontal: ScrollBar { policy: ScrollBar.AsNeeded }

                    PrimoEditor {
                        id: primo2
                        width: flick2.width
                        height: contentHeight
                        focus: true
                        gutterVisible: true
                        relativeNumbers: false
                        backgroundColor: palette.base
                        foregroundColor: palette.text
                        font.family: "Monospace"
                        font.pixelSize: 13
                        Component.onCompleted: {
                            if (tabs && tabs.count>0) {
                                var f = tabs.pane2File()
                                if (f) loadFile(f)
                            }
                        }
                        onDiagnosticsChanged: minimap2.requestPaint()
                        onBookmarksChanged: minimap2.requestPaint()
                    }
                }

                Minimap {
                    id: minimap
                    visible: primo.minimapVisible
                    Layout.preferredWidth: 100
                    Layout.fillHeight: true
                    editor: primo.activeFocus ? primo : (primo2.activeFocus ? primo2 : primo)
                    flick: primo.activeFocus ? flick : (primo2.activeFocus ? flick2 : flick)
                    bg: palette.base
                }
                // Second minimap for split? Reuse same, but when split show second minimap?
                Minimap {
                    id: minimap2
                    visible: tabs && tabs.splitVisible && primo2.minimapVisible
                    Layout.preferredWidth: 80
                    Layout.fillHeight: true
                    editor: primo2
                    flick: flick2
                    bg: palette.base
                }
            }
            // Second TabBar for pane 2 when split
            DraggableTabBar {
                id: tabBar2
                Layout.fillWidth: true
                visible: tabs ? tabs.splitVisible : false
                tabModel: tabs
                paneId: 1
                editor: primo2
            }
            RowLayout {
                Layout.fillWidth: true
                spacing: 8
                CheckBox { text: "Gutter"; checked: primo.gutterVisible; onToggled: { primo.gutterVisible = checked; if(primo2) primo2.gutterVisible = checked } }
                CheckBox { text: "Rel. numbers"; checked: primo.relativeNumbers; onToggled: { primo.relativeNumbers = checked; if(primo2) primo2.relativeNumbers = checked } }
                CheckBox { text: "Minimap"; checked: primo.minimapVisible; onToggled: { primo.minimapVisible = checked; if(primo2) primo2.minimapVisible = checked } }
                CheckBox { text: "Read-only"; checked: primo.readOnly; onToggled: { primo.readOnly = checked; if(primo2) primo2.readOnly = checked } }
                Label { text: "F2/Ctrl+B bookmark • drag tabs between panes"; opacity: 0.5; font.pixelSize: 10; Layout.fillWidth: true }
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
            tabs.addTab(path);
            actions.openFile(path);
            // also load into active editor
            if (primo.activeFocus || !primo2.activeFocus) primo.loadFile(path); else primo2.loadFile(path);
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
            var ed = primo.activeFocus ? primo : (primo2.activeFocus ? primo2 : primo)
            ed.saveAs(path);
            tabs.addTab(path);
        }
    }

    Dialog {
        id: gotoDlg
        title: "Go to Line"
        modal: true
        standardButtons: Dialog.Ok | Dialog.Cancel
        ColumnLayout {
            anchors.fill: parent
            Label { text: "Line number:" }
            SpinBox {
                id: gotoSpin
                from: 1; to: primo.document ? primo.document.lineCount : 999999
                value: (primo.activeFocus ? primo.cursorLine : (primo2.activeFocus ? primo2.cursorLine : 0)) + 1
                editable: true
            }
        }
        onAccepted: {
            var ed = primo.activeFocus ? primo : (primo2.activeFocus ? primo2 : primo)
            var fl = ed === primo ? flick : flick2
            ed.setCursorPosition(gotoSpin.value - 1, 0);
            ed.forceActiveFocus();
            fl.contentY = Math.max(0, (gotoSpin.value - 1) * ed.lineHeight - fl.height/2);
        }
    }

    Dialog {
        id: aboutDlg
        title: "About Devpad Nextgen"
        modal: true
        standardButtons: Dialog.Ok
        Label {
            text: "Devpad Nextgen — primoEditor " + devpadVersion + "\nQSG high-perf, gutter + minimap + async highlight + FindBar + draggable split tabs + 17 themes\n\nSemagsoft 2026"
            wrapMode: Text.Wrap
        }
    }

    Dialog {
        id: optionsDlg
        title: "Options"
        modal: true
        standardButtons: Dialog.Close
        ColumnLayout {
            anchors.fill: parent
            Label { text: "Theme:" }
            ComboBox {
                id: themeCombo
                Layout.fillWidth: true
                model: actions ? actions.themeNames : []
                currentIndex: {
                    if (!actions || !themeCombo.model) return 0;
                    var cur = actions.currentThemeName;
                    if (cur === undefined || cur === null) return 0;
                    var idx = themeCombo.model.indexOf(cur);
                    return idx >=0 ? idx : 0;
                }
                onActivated: (idx) => {
                    if (!themeCombo.model || idx <0 || idx >= themeCombo.model.length) return;
                    var name = themeCombo.model[idx];
                    actions.setThemeByName(name);
                }
            }
            Label { text: "Options: toolbar/statusbar persistence shared with Widgets."; wrapMode: Text.Wrap; opacity: 0.7 }
        }
    }

    Connections {
        target: actions
        function onRequestOpenFileDialog() { openFileDlg.open(); }
        function onRequestOpenFolderDialog() { openFolderDlg.open(); }
        function onRequestSaveAsDialog() { saveAsDlg.open(); }
        function onRequestGoToLineDialog() { gotoDlg.open(); }
        function onRequestFindDialog() {
            // Target active editor
            var ed = primo2 && primo2.activeFocus ? primo2 : primo;
            var fl = ed === primo2 ? flick2 : flick;
            findBar.editor = ed;
            findBar.flick = fl;
            findBar.show(true);
        }
        function onRequestReplaceDialog() {
            var ed = primo2 && primo2.activeFocus ? primo2 : primo;
            var fl = ed === primo2 ? flick2 : flick;
            findBar.editor = ed;
            findBar.flick = fl;
            findBar.show(false);
        }
        function onRequestAboutDialog() { aboutDlg.open(); }
        function onRequestOptionsDialog() { optionsDlg.open(); }
        function onShowMessage(msg) {
            statusMsg.text = msg;
            statusMsgTimer.restart();
        }
    }

    // Terminal Drawer – shared QProcess, simple TextArea shell
    Drawer {
        id: terminalDrawer
        edge: Qt.BottomEdge
        width: win.width
        height: win.height * 0.35
        dim: false
        modal: false
        interactive: true
        TerminalPane {
            anchors.fill: parent
            terminal: terminalInstance
            editor: primo.activeFocus ? primo : (primo2 && primo2.activeFocus ? primo2 : primo)
        }
    }

    // Find-in-Files Drawer – respects .gitignore + showHidden
    FindInFilesDrawer {
        id: findInFilesDrawer
        editor: primo.activeFocus ? primo : (primo2 && primo2.activeFocus ? primo2 : primo)
        folder: win.folder
        finder: finderInstance
    }

    // Sync tabModel current file to editor
    Connections {
        target: tabs
        function onCurrentIndexChanged() {
            var f = tabs.currentFile();
            if (f && primo.filePath !== f) primo.loadFile(f);
        }
        function onPane2IndexChanged() {
            var f = tabs.pane2File();
            if (f && primo2.filePath !== f) primo2.loadFile(f);
        }
    }

    // Terminal / Find-in-Files toggle via actions
    Connections {
        target: actions
        function onRequestTerminalToggle() {
            if (terminalDrawer.visible) terminalDrawer.close()
            else terminalDrawer.open()
        }
        function onRequestFindInFiles() { findInFilesDrawer.open() }
    }

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
    }

    Shortcut { sequence: "Ctrl+S"; onActivated: actions.saveFile() }
    Shortcut { sequence: "Ctrl+Shift+S"; onActivated: actions.saveFileAsDialog() }
    Shortcut { sequence: "Ctrl+O"; onActivated: actions.openFileDialog() }
    Shortcut { sequence: "Ctrl+G"; onActivated: actions.goToLine() }
    Shortcut { sequence: "Ctrl+F"; onActivated: { var ed = primo2 && primo2.activeFocus ? primo2 : primo; var fl = ed === primo2 ? flick2 : flick; findBar.editor = ed; findBar.flick = fl; findBar.show(true) } }
    Shortcut { sequence: "Ctrl+H"; onActivated: { var ed = primo2 && primo2.activeFocus ? primo2 : primo; var fl = ed === primo2 ? flick2 : flick; findBar.editor = ed; findBar.flick = fl; findBar.show(false) } }
    Shortcut { sequence: "Ctrl+B"; onActivated: actions.toggleBookmark() }
    Shortcut { sequence: "F2"; onActivated: actions.toggleBookmark() }
    Shortcut { sequence: "Ctrl+Z"; onActivated: actions.undo() }
    Shortcut { sequence: "Ctrl+Y"; onActivated: actions.redo() }
    Shortcut { sequence: "Ctrl+Shift+Z"; onActivated: actions.redo() }
}
