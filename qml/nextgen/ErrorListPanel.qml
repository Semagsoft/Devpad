import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

// ErrorListPanel: dedicated diagnostics panel for NextGen.
// Shows LSP diagnostics (errors/warnings) with click-to-navigate.
Pane {
    id: root
    property var editor: null
    property var diagnostics: [] // list of {uri, range, message, severity, severityLevel, source, code}

    padding: 6
    background: Rectangle { color: palette.base; border.color: palette.mid; border.width: 1 }

    ColumnLayout {
        anchors.fill: parent
        spacing: 8

        RowLayout {
            Layout.fillWidth: true
            spacing: 6
            Label {
                text: qsTr("Diagnostics")
                font.bold: true
                Layout.fillWidth: true
            }
            Label {
                id: diagCount
                text: diagnostics ? diagnostics.length + " issues" : "0 issues"
                opacity: 0.6
                font.pixelSize: 10
            }
            ToolButton {
                text: "×"
                onClicked: root.close()
            }
        }

        ScrollView {
            Layout.fillWidth: true
            Layout.fillHeight: true
            clip: true
            ListView {
                id: diagList
                width: parent.width
                height: contentHeight
                model: diagnostics
                delegate: ItemDelegate {
                    width: diagList.width
                    text: modelData.message
                    ToolTip.text: modelData.source ? modelData.source + ": " + modelData.message : modelData.message
                    property int sev: modelData.severityLevel || 1
                    RowLayout {
                        anchors.fill: parent
                        spacing: 6
                        Rectangle {
                            width: 4; height: 4
                            radius: 2
                            color: sev === 1 ? "#ff5555" : (sev === 2 ? "#ffb86c" : (sev === 3 ? "#f1fa8c" : "#8be9fd"))
                        }
                        Label {
                            text: modelData.message
                            Layout.fillWidth: true
                            elide: Text.ElideRight
                            font.pixelSize: 11
                        }
                        Label {
                            text: qsTr("L%1").arg(modelData.range ? modelData.range.start.line + 1 : 0)
                            font.pixelSize: 10
                            opacity: 0.6
                            width: 40
                        }
                    }
                    onClicked: {
                        var ed = root.editor
                        if (!ed) return
                        var loc = modelData
                        var uri = loc.uri || ""
                        var path = uri
                        if (path.startsWith("file://")) path = path.substring(7)
                        path = decodeURIComponent(path)
                        ed.loadFile(path)
                        ed.setCursorPosition(loc.range.start.line, loc.range.start.character)
                        ed.forceActiveFocus()
                        root.close()
                    }
                }
            }
        }
    }
}
