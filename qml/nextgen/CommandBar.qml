import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

// Inline command bar – type : commands at the bottom of the window.
// Modeled after FindBar.qml. Emits commandEntered(string) when the user presses Enter.
Pane {
    id: root
    property var actions: null
    property var editor: null

    visible: false
    padding: 4
    background: Rectangle { color: palette.alternateBase; border.color: palette.mid; border.width: 1; radius: 4 }

    signal commandEntered(string cmd)

    function show() {
        visible = true
        cmdField.forceActiveFocus()
        cmdField.selectAll()
    }
    function hideBar() { visible = false; if (editor) editor.forceActiveFocus(); }

    ColumnLayout {
        anchors.fill: parent
        spacing: 4
        RowLayout {
            Layout.fillWidth: true
            spacing: 6
            Label { text: ":"; font.pixelSize: 11; opacity: 0.8 }
            TextField {
                id: cmdField
                Layout.fillWidth: true
                placeholderText: "Enter command (e.g., :w, :q, :set number on, :help)"
                onAccepted: {
                    var c = cmdField.text.trimmed()
                    if (c.startsWith(":"))
                        c = c.mid(1)
                    root.commandEntered(c)
                    hideBar()
                }
                Keys.onEscapePressed: root.hideBar()
            }
            ToolButton {
                icon.source: "qrc:/icons/File/close.svg"
                ToolTip.text: "Close (Esc)"
                onClicked: root.hideBar()
            }
        }
        RowLayout {
            Layout.fillWidth: true
            spacing: 8
            Label { id: cmdStatus; Layout.fillWidth: true; opacity: 0.7; font.pixelSize: 11 }
        }
    }

    Keys.onPressed: (event) => {
        if (event.key === Qt.Key_Escape) { hideBar(); event.accepted = true }
    }
}
