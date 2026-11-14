import QtQuick
import QtQuick.Controls

// Cross-platform MenuBar: on macOS Qt.labs.platform would give native menu,
// but we use Controls MenuBar for consistency (Wayland/Linux). Native on macOS
// is handled via ApplicationWindow's menuBar still being Controls – Qt maps to native automatically.
MenuBar {
    id: root
    property var actions: null
    property var editor: null
    visible: actions ? actions.showMenuBar : true

    // File
    Menu {
        title: qsTr("&File")
        Action { text: qsTr("&New"); shortcut: "Ctrl+N"; onTriggered: actions.newFile() }
        Action { text: qsTr("New &Window"); shortcut: "Ctrl+Shift+N"; onTriggered: actions.newWindow() }
        MenuSeparator {}
        Action { text: qsTr("&Open..."); shortcut: "Ctrl+O"; onTriggered: actions.openFileDialog() }
        Action { text: qsTr("Open &Folder..."); shortcut: "Ctrl+Shift+O"; onTriggered: actions.openFolderDialog() }
        Action { text: qsTr("Open Remote &File..."); onTriggered: actions.openRemoteDialog() }
        Action { text: qsTr("Open Remote &Folder..."); onTriggered: actions.openRemoteFolderDialog() }
        MenuSeparator {}
        Menu {
            title: qsTr("Recent Files")
            Repeater {
                model: actions ? actions.recentFiles.slice(0, 10) : []
                MenuItem {
                    text: modelData.split("/").pop() + "  —  " + modelData
                    onTriggered: actions.openRecentFile(modelData)
                }
            }
            MenuSeparator {}
            Action { text: qsTr("Clear Recent Files"); onTriggered: actions.clearRecentFiles() }
        }
        MenuSeparator {}
        Action { text: qsTr("&Save"); shortcut: "Ctrl+S"; enabled: editor && editor.filePath !== ""; onTriggered: actions.saveFile() }
        Action { text: qsTr("Save &As..."); shortcut: "Ctrl+Shift+S"; onTriggered: actions.saveFileAsDialog() }
        MenuSeparator {}
        Action { text: qsTr("&Print..."); shortcut: "Ctrl+P"; onTriggered: actions.printFile() }
        Action { text: qsTr("Print &Preview..."); onTriggered: actions.printPreview() }
        Action { text: qsTr("Page &Setup..."); onTriggered: actions.pageSetup() }
        MenuSeparator {}
        Action { text: qsTr("Close &File"); shortcut: "Ctrl+W"; onTriggered: actions.closeFile() }
        Action { text: qsTr("Close &All Tabs"); shortcut: "Ctrl+Shift+W"; onTriggered: actions.closeAllTabs() }
        Action { text: qsTr("Close &Project"); onTriggered: actions.closeProject() }
        MenuSeparator {}
        Action { text: qsTr("E&xit"); shortcut: "Ctrl+Q"; onTriggered: actions.exitApp() }
    }

    Menu {
        title: qsTr("&Edit")
        Action { text: qsTr("&Undo"); shortcut: "Ctrl+Z"; onTriggered: actions.undo() }
        Action { text: qsTr("&Redo"); shortcut: "Ctrl+Y"; onTriggered: actions.redo() }
        MenuSeparator {}
        Action { text: qsTr("Cu&t"); shortcut: "Ctrl+X"; onTriggered: actions.cut() }
        Action { text: qsTr("&Copy"); shortcut: "Ctrl+C"; onTriggered: actions.copy() }
        Action { text: qsTr("&Paste"); shortcut: "Ctrl+V"; onTriggered: actions.paste() }
        Action { text: qsTr("Select &All"); shortcut: "Ctrl+A"; onTriggered: actions.selectAll() }
        MenuSeparator {}
        Action { text: qsTr("&Find..."); shortcut: "Ctrl+F"; onTriggered: actions.find() }
        Action { text: qsTr("&Replace..."); shortcut: "Ctrl+H"; onTriggered: actions.replace() }
        Action { text: qsTr("Find &Next"); shortcut: "F3"; onTriggered: actions.findNext() }
        Action { text: qsTr("Find &Previous"); shortcut: "Shift+F3"; onTriggered: actions.findPrevious() }
        Action { text: qsTr("Find in &Files..."); shortcut: "Ctrl+Shift+F"; onTriggered: actions.findInFiles() }
        Action { text: qsTr("&Go to Line..."); shortcut: "Ctrl+G"; onTriggered: actions.goToLine() }
        MenuSeparator {}
        // ── LSP ──
        Action { text: qsTr("Find &References"); shortcut: "Shift+F12"; onTriggered: {
            var ed = primo.activeFocus ? primo : (primo2 && primo2.activeFocus ? primo2 : primo)
            if (ed && ed.filePath) {
                var uri = "file://" + ed.filePath
                lspClient.requestReferencesAt(uri, ed.cursorLine, ed.cursorColumn)
            }
        } }
        Action { text: qsTr("&Format Document"); shortcut: "Shift+Alt+F"; onTriggered: {
            var ed = primo.activeFocus ? primo : (primo2 && primo2.activeFocus ? primo2 : primo)
            if (ed && ed.filePath) {
                var uri = "file://" + ed.filePath
                lspClient.requestFormatting(uri, 4, true)
            }
        } }
        Action { text: qsTr("&Rename Symbol"); shortcut: "Ctrl+Shift+R"; onTriggered: renameDlg.open() }
        Action { text: qsTr("&Code Actions"); shortcut: "Ctrl+Shift+A"; onTriggered: {
            var ed = primo.activeFocus ? primo : (primo2 && primo2.activeFocus ? primo2 : primo)
            if (ed && ed.filePath) {
                var uri = "file://" + ed.filePath
                var range = {start: {line: ed.cursorLine, character: ed.cursorColumn}, end: {line: ed.cursorLine, character: ed.cursorColumn}}
                lspClient.requestCodeAction(uri, range, [])
            }
        } }
        Action { text: qsTr("&Document Symbols"); shortcut: "Ctrl+Shift+O"; onTriggered: {
            var ed = primo.activeFocus ? primo : (primo2 && primo2.activeFocus ? primo2 : primo)
            if (ed && ed.filePath) {
                var uri = "file://" + ed.filePath
                lspClient.requestDocumentSymbols(uri)
            }
        } }
        MenuSeparator {}
        Action { text: qsTr("Toggle &Comment"); shortcut: "Ctrl+/"; onTriggered: actions.showMessage ? actions.showMessage("Toggle comment") : console.log("comment") }
        MenuSeparator {}
        Action { text: qsTr("Toggle &Bookmark"); shortcut: "Ctrl+F2"; onTriggered: actions.toggleBookmark() }
        Action { text: qsTr("&Next Bookmark"); shortcut: "F2"; onTriggered: actions.nextBookmark() }
        Action { text: qsTr("&Previous Bookmark"); shortcut: "Shift+F2"; onTriggered: actions.prevBookmark() }
        Action { text: qsTr("&Clear Bookmarks"); onTriggered: actions.clearBookmarks() }
    }

    Menu {
        title: qsTr("&View")
        Action { text: qsTr("Show &Menu Bar"); checkable: true; checked: actions ? actions.showMenuBar : true; onTriggered: actions.showMenuBar = checked }
        Action { text: qsTr("Show &Tool Bar"); checkable: true; checked: actions ? actions.showToolbar : true; onTriggered: actions.showToolbar = checked }
        Action { text: qsTr("Show &Status Bar"); checkable: true; checked: actions ? actions.showStatusbar : true; onTriggered: actions.showStatusbar = checked }
        MenuSeparator {}
        Action { text: qsTr("Show &Gutter"); checkable: true; checked: editor ? editor.gutterVisible : true; onTriggered: editor.gutterVisible = checked }
        Action { text: qsTr("Relative Line Numbers"); checkable: true; checked: editor ? editor.relativeNumbers : false; onTriggered: editor.relativeNumbers = checked }
        Action { text: qsTr("Show &Minimap"); checkable: true; checked: editor ? editor.minimapVisible : true; onTriggered: editor.minimapVisible = checked }
        Action { text: qsTr("&Word Wrap"); checkable: true; checked: editor ? editor.wordWrap : false; onTriggered: editor.wordWrap = checked; shortcut: "Alt+Z" }
        Action { text: qsTr("&Terminal"); shortcut: "Ctrl+`"; checkable: true; onTriggered: actions.toggleTerminal() }
        Action { text: qsTr("&Error List"); shortcut: "Ctrl+Shift+E"; onTriggered: actions.toggleErrorList() }
        MenuSeparator {}
        Action { text: qsTr("Zoom &In"); shortcut: "Ctrl++"; onTriggered: actions.zoomIn() }
        Action { text: qsTr("Zoom &Out"); shortcut: "Ctrl+-"; onTriggered: actions.zoomOut() }
        Action { text: qsTr("Zoom &Reset"); shortcut: "Ctrl+0"; onTriggered: actions.zoomReset() }
        Action { text: qsTr("Toggle &Full Screen"); shortcut: "F11"; onTriggered: actions.toggleFullScreen() }
        MenuSeparator {}
        Action { text: qsTr("Read &Only"); checkable: true; checked: editor ? editor.readOnly : false; onTriggered: editor.readOnly = checked }
        MenuSeparator {}
        Menu {
            title: qsTr("&Theme")
            Repeater {
                model: actions ? actions.themeNames : []
                MenuItem {
                    text: modelData
                    checkable: true
                    checked: modelData === (actions ? actions.currentThemeName : "")
                    onTriggered: actions.setThemeByName(modelData)
                }
            }
        }
    }

    Menu {
        title: qsTr("&Tools")
        Action { text: qsTr("&Options..."); onTriggered: actions.showOptions() }
        Action { text: qsTr("Configure External Tools..."); onTriggered: actions.showOptions() }
    }

    Menu {
        title: qsTr("&Help")
        Action { text: qsTr("&About Devpad"); onTriggered: actions.showAbout() }
        Action { text: qsTr("Check for &Updates"); onTriggered: actions.checkForUpdates() }
        Action { text: qsTr("&Website"); onTriggered: Qt.openUrlExternally("https://semagsoft.com") }
    }
}
