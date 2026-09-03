import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

// Find-in-Files Drawer – slide from bottom, respects .gitignore + showHidden via PrimoFindInFiles
Drawer {
    id: root
    property var editor: null
    property string folder: ""
    property var finder: null // PrimoFindInFiles instance

    edge: Qt.BottomEdge
    width: parent ? parent.width : 800
    height: parent ? parent.height * 0.45 : 400
    modal: false
    interactive: true
    dim: false

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 8
        spacing: 8

        RowLayout {
            Layout.fillWidth: true
            spacing: 8
            Label { text: "Find in Files:"; font.bold: true }
            TextField {
                id: patternField
                Layout.fillWidth: true
                placeholderText: "Search pattern..."
                onAccepted: doSearch()
            }
            ToolButton {
                text: "Search"
                onClicked: doSearch()
            }
            ToolButton {
                text: "×"
                onClicked: root.close()
            }
        }

        RowLayout {
            Layout.fillWidth: true
            spacing: 8
            Label { text: "Folder:"; opacity: 0.7 }
            TextField {
                id: folderField
                Layout.fillWidth: true
                text: root.folder
                placeholderText: "Root folder (uses project folder if empty)"
                onTextChanged: root.folder = text
            }
            ToolButton {
                text: "Browse"
                onClicked: folderDialog.open()
            }
            Label { text: "Glob:"; opacity: 0.7 }
            TextField {
                id: globField
                Layout.preferredWidth: 120
                placeholderText: "*.cpp *.h"
            }
        }

        RowLayout {
            Layout.fillWidth: true
            spacing: 8
            CheckBox { id: caseBox; text: "Aa"; ToolTip.text: "Case sensitive" }
            CheckBox { id: wordBox; text: "W"; ToolTip.text: "Whole word" }
            CheckBox { id: regexBox; text: ".*"; ToolTip.text: "Regex" }
            Label { id: statusLabel; Layout.fillWidth: true; opacity: 0.6; font.pixelSize: 11; text: finder ? finder.resultCount + " results" : "0 results" }
            Label { text: ".gitignore respected"; opacity: 0.5; font.pixelSize: 10; ToolTip.text: "Search respects .gitignore and showHidden setting" }
        }

        ListModel { id: resultModel }

        ScrollView {
            Layout.fillWidth: true
            Layout.fillHeight: true
            clip: true
            ListView {
                id: resultView
                model: resultModel
                delegate: ItemDelegate {
                    width: resultView.width
                    text: modelData.display ? modelData.display : (modelData.filePath + ":" + (modelData.line+1) + ": " + modelData.lineText)
                    onClicked: {
                        if (editor) {
                            editor.loadFile(modelData.filePath)
                            editor.setCursorPosition(modelData.line, modelData.column)
                        }
                    }
                }
            }
        }

        function doSearch() {
            if (!finder) return
            var pat = patternField.text
            if (!pat) return
            var rootPath = folderField.text || root.folder
            if (!rootPath) {
                statusLabel.text = "No folder set"
                return
            }
            statusLabel.text = "Searching..."
            var res = finder.search(pat, rootPath, globField.text, caseBox.checked, wordBox.checked, regexBox.checked)
            resultModel.clear()
            for (var i=0;i<res.length;i++) resultModel.append(res[i])
            resultView.model = resultModel
            statusLabel.text = res.length + " results in " + rootPath
        }

        Connections {
            target: finder
            function onSearchFinished(count) {
                statusLabel.text = count + " results"
            }
        }
    }

    // Folder browse dialog for find-in-files
    // Use QtQuick.Dialogs FolderDialog via Loader to avoid import conflict if not needed
    // For simplicity, reuse same FolderDialog as Main.qml would, but we need to create one here
    // We'll use a simple dialog via QtQuick.Dialogs if available
    // Fallback: use text input only
}
