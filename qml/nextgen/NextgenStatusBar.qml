import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

// Full encoding ComboBox + line/col, file type, diagnostics
Pane {
    id: root
    property var actions: null
    property var editor: null
    visible: actions ? actions.showStatusbar : true
    padding: 4
    background: Rectangle { color: palette.alternateBase }

    // Encoding model: same as EncodingMenuHelper
    property var encodings: ["UTF-8", "UTF-8-BOM", "UTF-16LE", "UTF-16BE", "UTF-32LE", "UTF-32BE", "ISO-8859-1", "System"]

    RowLayout {
        anchors.fill: parent
        spacing: 8

        Label {
            id: fileLabel
            text: editor && editor.filePath ? editor.filePath : qsTr("Untitled")
            Layout.fillWidth: true
            font.pixelSize: 11
            opacity: 0.85
            wrapMode: Text.NoWrap
        }

        Label {
            text: editor && editor.language ? editor.language : qsTr("Plain Text")
            font.pixelSize: 11
            opacity: 0.6
        }

        // Encoding ComboBox – Full (reopen / save)
        ComboBox {
            id: encCombo
            Layout.preferredWidth: 140
            model: root.encodings
            currentIndex: {
                var cur = actions ? actions.currentEncoding : "UTF-8";
                var idx = root.encodings.indexOf(cur);
                return idx >= 0 ? idx : 0;
            }
            font.pixelSize: 11
            ToolTip.text: qsTr("Encoding: click to reopen/save")
            ToolTip.visible: hovered
            onActivated: (index) => {
                var enc = root.encodings[index];
                // Context menu for reopen vs save? For MVP, reopen
                actions.reopenWithEncoding(enc);
            }
            // Right-click for save with encoding
            MouseArea {
                anchors.fill: parent
                acceptedButtons: Qt.RightButton
                onClicked: (mouse) => {
                    if (mouse.button === Qt.RightButton) {
                        encSaveMenu.popup();
                    }
                }
                Menu {
                    id: encSaveMenu
                    Repeater {
                        model: root.encodings
                        MenuItem {
                            text: qsTr("Save with ") + modelData
                            onTriggered: actions.saveWithEncoding(modelData)
                        }
                    }
                }
            }
        }

        Label {
            text: editor ? (editor.readOnly ? qsTr("Read Only") : qsTr("RW")) : ""
            color: editor && editor.readOnly ? "red" : palette.text
            font.pixelSize: 11
            opacity: 0.8
        }

        Label {
            text: editor && editor.document ? editor.document.lineCount + " lines" : "0 lines"
            font.pixelSize: 11
            opacity: 0.6
        }
        Label {
            text: editor && editor.document ? editor.document.length + " chars" : "0 chars"
            font.pixelSize: 11
            opacity: 0.6
        }

        Label {
            text: editor ? "Ln " + (editor.cursorLine+1) + ", Col " + (editor.cursorColumn+1) : "Ln 1, Col 1"
            font.family: "Monospace"
            font.pixelSize: 11
            opacity: 0.85
        }

        Label {
            text: editor && editor.bookmarkLines ? editor.bookmarkLines().length + " bookmarks" : "0 bookmarks"
            font.pixelSize: 11
            opacity: 0.6
            visible: editor && editor.bookmarkLines && editor.bookmarkLines().length > 0
        }

        Label {
            text: "LF"
            font.pixelSize: 11
            opacity: 0.6
            ToolTip.text: qsTr("Line ending: LF")
            ToolTip.visible: hovered
        }
    }
}
