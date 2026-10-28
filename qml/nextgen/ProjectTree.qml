import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import Devpad.Nextgen 1.0

// ProjectTree: file tree view for NextGen sidebar.
// Respects .gitignore and showHidden via FileTreeModel (C++).
Pane {
    id: root
    property var model: fileTreeModel
    property string rootPath: ""
    property var editor: null

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
            Label {
                text: model ? model.count + " items" : "0 items"
                opacity: 0.6
                font.pixelSize: 10
                Layout.fillWidth: true
                alignment: Qt.AlignRight
            }
        }
    }

    // Folder browse dialog
    FolderDialog {
        id: folderDialog
        title: qsTr("Open Project Folder")
        onAccepted: {
            var path = selectedFolder.toString().replace("file://", "");
            path = decodeURIComponent(path);
            root.rootPath = path;
            model.setRootPath(path);
        }
    }

    Component.onCompleted: {
        if (rootPath) {
            model.setRootPath(rootPath);
        }
    }
}