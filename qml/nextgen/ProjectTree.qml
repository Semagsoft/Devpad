import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Dialogs 6.3
import Devpad.Nextgen 1.0

// ProjectTree: file tree view for NextGen sidebar.
// Respects .gitignore and showHidden via FileTreeModel (C++).
Pane {
    id: root
    property var fileModel: fileTreeModel
    property string rootPath: ""
    property var editor: null
    property var tabs: null
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
                checked: fileModel ? fileModel.showHidden : false
                onToggled: if (fileModel) fileModel.showHidden = checked
                ToolTip.text: qsTr("Show hidden files")
                font.pixelSize: 10
            }
            CheckBox {
                text: qsTr("GitIgnore")
                checked: fileModel ? fileModel.useGitIgnore : true
                onToggled: if (fileModel) fileModel.useGitIgnore = checked
                ToolTip.text: qsTr("Respect .gitignore")
                font.pixelSize: 10
            }
        }

        ScrollView {
            Layout.fillWidth: true
            Layout.fillHeight: true
            clip: true
            TreeView {
                id: fileList
                anchors.fill: parent
                model: fileTreeModel
                delegate: Item {
                    id: treeDelegate
                    implicitWidth: fileList.width
                    implicitHeight: 24
                    required property var model
                    required property int depth
                    Row {
                        anchors.verticalCenter: parent.verticalCenter
                        x: depth * 12
                        spacing: 6
                        Text { text: model.isDir ? "📁" : "📄"; font.pixelSize: 12 }
                        Text {
                            text: model.fileName
                            elide: Text.ElideMiddle
                            font.pixelSize: 12
                            color: model.isDir ? palette.highlightedText : palette.windowText
                        }
                    }
                    TapHandler {
                        onTapped: {
                            if (!model.isDir && editor) {
                                editor.loadFile(model.filePath)
                                if (tabs) tabs.addTab(model.filePath)
                            }
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
                onClicked: if (fileModel) fileModel.refresh()
            }
            ToolButton {
                text: "New Folder"
                onClicked: newFolderDialog.open()
            }
            ToolButton {
                text: "Rename"
                onClicked: {
                    var idx = fileList.currentIndex
                    if (idx >= 0 && fileModel) renameDialog.open()
                }
            }
            ToolButton {
                text: "Delete"
                onClicked: {
                    var idx = fileList.currentIndex
                    if (idx >= 0 && fileModel) {
                        root.deletePath = fileModel.filePath(idx)
                        deleteDialog.open()
                    }
                }
            }
            Label {
                text: fileList.count + " items"
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
            if (fileModel) fileModel.rootPath = path
        }
    }

    // New folder dialog
    InputDialog {
        id: newFolderDialog
        title: qsTr("New Folder")
        label: qsTr("Folder name:")
        onAccepted: {
            var name = textInput.text.trimmed()
            if (name && fileModel) fileModel.newFolder(name)
        }
    }

    // Rename dialog
    InputDialog {
        id: renameDialog
        title: qsTr("Rename")
        label: qsTr("New name:")
        property string oldPath: ""
        onAccepted: {
            var newName = textInput.text.trimmed()
            if (newName && oldPath && fileModel) {
                var dir = oldPath.substring(0, oldPath.lastIndexOf("/") + 1)
                fileModel.renamePath(oldPath, dir + newName)
            }
        }
        onOpened: {
            var idx = fileList.currentIndex
            if (idx >= 0 && fileModel) {
                oldPath = fileModel.filePath(idx)
                textInput.text = fileModel.fileName(idx)
            }
        }
    }

    // Delete confirmation dialog
    MessageDialog {
        id: deleteDialog
        title: qsTr("Delete")
        text: qsTr("Delete %1?").arg(root.deletePath.split("/").pop())
        buttons: MessageDialog.Yes | MessageDialog.No
        onAccepted: {
            if (root.deletePath && fileModel) fileModel.remove(root.deletePath)
            root.deletePath = ""
        }
        onRejected: root.deletePath = ""
    }

    onRootPathChanged: {
        if (rootPath && fileModel) fileModel.rootPath = rootPath
    }
    Component.onCompleted: {
        if (rootPath && fileModel) fileModel.rootPath = rootPath
    }
}