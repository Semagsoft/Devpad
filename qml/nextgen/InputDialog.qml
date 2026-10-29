import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

// Simple text-input dialog used by ProjectTree (New Folder / Rename).
// Exposes textInput (a TextField) and emits accepted()/rejected().
Dialog {
    id: root
    property string label: ""

    readonly property alias textInput: textField

    modal: true
    standardButtons: Dialog.Ok | Dialog.Cancel
    width: 360

    ColumnLayout {
        spacing: 8
        Label { text: root.title; font.bold: true }
        Label { text: root.label; opacity: 0.8; font.pixelSize: 11 }
        TextField {
            id: textField
            Layout.fillWidth: true
            onAccepted: root.accept()
        }
    }

    onOpened: {
        textField.forceActiveFocus()
        textField.selectAll()
    }
}
