import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Dialogs 6.3
import Devpad.Nextgen 1.0

// ProjectTree: file tree view for NextGen sidebar.
// Respects .gitignore and showHidden via FileTreeModel (C++).
Pane {
    id: root
    property var model: fileTreeModel
    property string rootPath: ""
    property var editor: null
    property string deletePath: ""

    padding: 6
    background: Rectangle { color: palette.base; border.color: palette.mid; border.width: 1 }

    ColumnLayout {
        anchors.fill: parent
        spacing: 8

        RowLayout {
            Layout.fillWidth: true
            spacing: 6
            Label { text: rootPath || "No folder"; Layout.fillWidth: true; font.pixelSize: 11; opacity: 0.8; elide: Text.ElideMiddle }
            ToolButton {
                text: "Browse"
                onClicked: folderDialog.open()
            }
            CheckBox {
                text: qsTr("Hidden")
                checked: model ? model.showHidden : false
                onToggled: model.setShowHidden(checked)
                ToolTip.text: qsTr("Show hidden files")
                font.pixelSize: 10
            }
            CheckBox {
                text: qsTr("GitIgnore")
                checked: model ? model.useGitIgnore : true
                onToggled: model.setUseGitIgnore(checked)
                ToolTip.text: qsTr("Respect .gitignore")
                font.pixelSize: 10
            }
        }

        ScrollView {
            Layout.fillWidth: true
            Layout.fillHeight: true
            clip: true
            ListView {
                id: fileList
                model: model
                delegate: ItemDelegate {
                    width: fileList.width
                    padding: 6
                    contentItem: Row {
                        spacing: 8
                        Item {
                            width: model.isDir ? 16 : 0
                            Text {
                                text: model.isDir ? "📁" : "📄"
                                font.pixelSize: 12
                                anchors.verticalCenter: parent.verticalCenter
                            }
                        }
                        Text {
                            text: model.fileName
                            elide: Text.ElideMiddle
                            font.pixelSize: 12
                            color: model.isDir ? palette.highlightedText : palette.windowText
                            anchors.verticalCenter: parent.verticalCenter
                            Layout.fillWidth: true
                        }
                    }
                    onClicked: {
                        if (!model.isDir && editor) {
                            editor.loadFile(model.filePath)
                        } else if (model.isDir) {
                            // For now, just refresh to show directory contents
                            // Full recursive tree would need a tree model
                            console.log("Directory: " + model.filePath)
                        }
                    }
                }
            }
        }

        RowLayout {
            Layout.fillWidth: true
            spacing: 4
            ToolButton {
                text: "Refresh"
                onClicked: model.refresh()
            }
            ToolButton {
                text: "New Folder"
                onClicked: newFolderDialog.open()
            }
            ToolButton {
                text: "Rename"
                onClicked: {
                    var idx = fileList.currentIndex
                    if (idx >= 0) renameInput.text = model.fileName(idx); renameDialog.open()
                }
            }
            ToolButton {
                text: "Delete"
                onClicked: {
                    var idx = fileList.currentIndex
                    if (idx >= 0) { deletePath = model.filePath(idx); deleteDialog.open() }
                }
            }
            Label {
                text: model ? model.count + " items" : "0 items"
                opacity: 0.6
                font.pixelSize: 10
                Layout.fillWidth: true
                Layout.alignment: Qt.AlignRight
            }
        }
    }

    // Folder browse dialog
    FolderDialog {
        id: folderDialog
        title: qsTr("Open Project Folder")
        currentFolder: root.rootPath ? "file://" + root.rootPath : ""
        onAccepted: {
            var path = selectedFolder.toString().replace("file://", "");
            path = decodeURIComponent(path);
            root.rootPath = path;
            model.setRootPath(path);
        }
    }

    // New folder dialog
    InputDialog {
        id: newFolderDialog
        title: qsTr("New Folder")
        label: qsTr("Folder name:")
        textInputMode: TextInput.NormalText
        onAccepted: {
            var name = textInput.text.trimmed()
            if (name && model) model.newFolder(name)
        }
    }

    // Rename dialog
    InputDialog {
        id: renameDialog
        title: qsTr("Rename")
        label: qsTr("New name:")
        textInputMode: TextInput.NormalText
        property string oldPath: ""
        onAccepted: {
            var newName = textInput.text.trimmed()
            if (newName && oldPath) {
                var dir = oldPath.substring(0, oldPath.lastIndexOf("/") + 1)
                model.rename(oldPath, dir + newName)
            }
        }
        onOpened: {
            var idx = fileList.currentIndex
            if (idx >= 0) {
                oldPath = model.filePath(idx)
                textInput.text = model.fileName(idx)
            }
        }
    }

    // Delete confirmation dialog
    MessageDialog {
        id: deleteDialog
        title: qsTr("Delete")
        icon: MessageDialog.Warning
        text: qsTr("Delete %1?").arg(root.deletePath.split("/").pop())
        standardButtons: StandardButtons.Yes | StandardButtons.No
        onOpened: {
            var idx = fileList.currentIndex
            if (idx >= 0) root.deletePath = model.filePath(idx)
        }
        onYes: {
            if (root.deletePath && model) model.remove(root.deletePath)
        }
    }

    Component.onCompleted: {
        if (rootPath) {
            model.setRootPath(rootPath);
        }
    }
}