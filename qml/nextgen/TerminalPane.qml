import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

// Simple QML TextArea shell via QProcess – shared process (PrimoTerminal singleton)
Pane {
    id: root
    property var terminal: null // PrimoTerminal instance
    property var editor: null // for currentDir sync
    padding: 6
    background: Rectangle { color: palette.base; border.color: palette.mid; border.width: 1 }

    ColumnLayout {
        anchors.fill: parent
        spacing: 4

        RowLayout {
            Layout.fillWidth: true
            spacing: 6
            Label { text: "Terminal — " + (terminal ? terminal.shellProgram : "shell") + " — " + (terminal ? terminal.currentDir : ""); Layout.fillWidth: true; font.pixelSize: 11; opacity: 0.8; elide: Text.ElideMiddle }
            Label { text: terminal && terminal.running ? "● running" : "○ stopped"; color: terminal && terminal.running ? "green" : "red"; font.pixelSize: 10 }
            ToolButton { text: "Clear"; onClicked: terminal.clearOutput() }
            ToolButton { text: terminal && terminal.running ? "Stop" : "Start"; onClicked: terminal.running ? terminal.stop() : terminal.start() }
            ToolButton { text: "Restart"; onClicked: terminal.restart() }
        }

        ScrollView {
            id: scroll
            Layout.fillWidth: true
            Layout.fillHeight: true
            clip: true
            TextArea {
                id: outputArea
                readOnly: true
                wrapMode: TextArea.Wrap
                font.family: "Monospace"
                font.pixelSize: 12
                text: terminal ? terminal.output : ""
                onTextChanged: {
                    // Auto-scroll to bottom
                    cursorPosition = text.length
                }
                // Selectable, copy
            }
        }

        RowLayout {
            Layout.fillWidth: true
            spacing: 6
            Label { text: "$"; font.family: "Monospace"; font.bold: true }
            TextField {
                id: inputField
                Layout.fillWidth: true
                placeholderText: "Type command and press Enter..."
                font.family: "Monospace"
                onAccepted: {
                    if (!terminal) return
                    var cmd = text
                    if (cmd.trim().length === 0) return
                    terminal.write(cmd + "\n")
                    text = ""
                }
                Keys.onUpPressed: {
                    // History not yet; placeholder
                }
            }
            ToolButton {
                text: "Send"
                onClicked: {
                    if (inputField.text.trim().length === 0) return
                    terminal.write(inputField.text + "\n")
                    inputField.text = ""
                }
            }
        }
    }

    // Sync currentDir from editor's folder
    Connections {
        target: editor
        function onFilePathChanged() {
            if (!terminal || !editor) return
            var fp = editor.filePath
            if (fp) {
                var dir = fp.substring(0, fp.lastIndexOf("/"))
                if (dir) terminal.currentDir = dir
            }
        }
    }

    Component.onCompleted: {
        if (terminal && !terminal.running) terminal.start()
    }
}
