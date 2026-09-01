import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

// Icon-only toolbar – persists showToolbar via SettingsManager
ToolBar {
    id: root
    property var actions: null
    property var editor: null
    visible: actions ? actions.showToolbar : true
    padding: 4

    RowLayout {
        anchors.fill: parent
        spacing: 2

        // Icon-only: no text, only icons + tooltips

        ToolButton {
            icon.source: "qrc:/icons/File/newfile.svg"
            ToolTip.text: qsTr("New (Ctrl+N)")
            ToolTip.visible: hovered
            display: AbstractButton.IconOnly
            onClicked: actions.newFile()
        }
        ToolButton {
            icon.source: "qrc:/icons/File/open.svg"
            ToolTip.text: qsTr("Open (Ctrl+O)")
            ToolTip.visible: hovered
            display: AbstractButton.IconOnly
            onClicked: actions.openFileDialog()
        }
        ToolButton {
            icon.source: "qrc:/icons/File/openfolder.svg"
            ToolTip.text: qsTr("Open Folder")
            ToolTip.visible: hovered
            display: AbstractButton.IconOnly
            onClicked: actions.openFolderDialog()
        }
        ToolSeparator {}

        ToolButton {
            icon.source: "qrc:/icons/File/save.svg"
            ToolTip.text: qsTr("Save (Ctrl+S)")
            ToolTip.visible: hovered
            enabled: editor && !editor.readOnly
            display: AbstractButton.IconOnly
            onClicked: actions.saveFile()
        }
        ToolButton {
            icon.source: "qrc:/icons/File/saveas.svg"
            ToolTip.text: qsTr("Save As (Ctrl+Shift+S)")
            ToolTip.visible: hovered
            display: AbstractButton.IconOnly
            onClicked: actions.saveFileAsDialog()
        }
        ToolSeparator {}

        ToolButton {
            icon.source: "qrc:/icons/Edit/undo.svg"
            ToolTip.text: qsTr("Undo (Ctrl+Z)")
            ToolTip.visible: hovered
            display: AbstractButton.IconOnly
            onClicked: actions.undo()
        }
        ToolButton {
            icon.source: "qrc:/icons/Edit/redo.svg"
            ToolTip.text: qsTr("Redo (Ctrl+Y)")
            ToolTip.visible: hovered
            display: AbstractButton.IconOnly
            onClicked: actions.redo()
        }
        ToolSeparator {}

        ToolButton {
            icon.source: "qrc:/icons/Edit/find.svg"
            ToolTip.text: qsTr("Find (Ctrl+F)")
            ToolTip.visible: hovered
            display: AbstractButton.IconOnly
            onClicked: actions.find()
        }
        ToolButton {
            icon.source: "qrc:/icons/Edit/replace.svg"
            ToolTip.text: qsTr("Replace (Ctrl+H)")
            ToolTip.visible: hovered
            display: AbstractButton.IconOnly
            onClicked: actions.replace()
        }
        ToolButton {
            icon.source: "qrc:/icons/Edit/goto.svg"
            ToolTip.text: qsTr("Go to Line (Ctrl+G)")
            ToolTip.visible: hovered
            display: AbstractButton.IconOnly
            onClicked: actions.goToLine()
        }
        ToolSeparator {}

        ToolButton {
            icon.source: "qrc:/icons/Edit/togglebookmark.svg"
            ToolTip.text: qsTr("Toggle Bookmark (Ctrl+F2)")
            ToolTip.visible: hovered
            display: AbstractButton.IconOnly
            onClicked: actions.toggleBookmark()
        }
        ToolButton {
            icon.source: "qrc:/icons/View/zoomin.svg"
            ToolTip.text: qsTr("Zoom In (Ctrl++)")
            ToolTip.visible: hovered
            display: AbstractButton.IconOnly
            onClicked: actions.zoomIn()
        }
        ToolButton {
            icon.source: "qrc:/icons/View/zoomout.svg"
            ToolTip.text: qsTr("Zoom Out (Ctrl+-)")
            ToolTip.visible: hovered
            display: AbstractButton.IconOnly
            onClicked: actions.zoomOut()
        }
        ToolButton {
            icon.source: "qrc:/icons/View/zoomreset.svg"
            ToolTip.text: qsTr("Zoom Reset (Ctrl+0)")
            ToolTip.visible: hovered
            display: AbstractButton.IconOnly
            onClicked: actions.zoomReset()
        }
        ToolButton {
            icon.source: "qrc:/icons/View/ui.svg"
            text: "Wrap"
            ToolTip.text: qsTr("Word Wrap (Alt+Z)")
            ToolTip.visible: hovered
            display: AbstractButton.IconOnly
            checkable: true
            checked: editor ? editor.wordWrap : false
            onClicked: editor.wordWrap = !editor.wordWrap
        }

        Item { Layout.fillWidth: true }

        ToolButton {
            icon.source: "qrc:/icons/Tools/options.svg"
            ToolTip.text: qsTr("Options")
            ToolTip.visible: hovered
            display: AbstractButton.IconOnly
            onClicked: actions.showOptions()
        }
    }
}
