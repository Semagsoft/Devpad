import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import Devpad.Nextgen 1.0

ApplicationWindow {
    id: win
    visible: true
    width: 1200
    height: 800
    title: "Devpad Nextgen — primoEditor " + devpadVersion
    color: palette.window

    property var openFiles: initialFiles
    property string folder: initialFolder

    header: ToolBar {
        RowLayout {
            anchors.fill: parent
            spacing: 8
            Label { text: "Devpad Nextgen (high-perf)"; font.bold: true; Layout.leftMargin: 8 }
            Label { text: "primoEditor • QSG"; opacity: 0.6; font.pixelSize: 11 }
            Label { text: "(" + Math.round(primo.lineHeight) + "px/line)"; opacity: 0.4; font.pixelSize: 10 }
            Item { Layout.fillWidth: true }
            Label { text: openFiles.length ? openFiles.length + " file(s)" : "No files"; opacity: 0.7 }
            Label { text: primo.cursorLine + ":" + primo.cursorColumn; opacity: 0.7; font.family: "Monospace" }
            ToolButton { text: "Save"; enabled: primo.filePath !== ""; onClicked: primo.save() }
        }
    }

    SplitView {
        anchors.fill: parent
        orientation: Qt.Horizontal

        Pane {
            SplitView.preferredWidth: 260
            SplitView.minimumWidth: 180
            padding: 6
            ColumnLayout {
                anchors.fill: parent
                spacing: 6
                Label { text: folder ? folder : "No folder"; elide: Text.ElideMiddle; Layout.fillWidth: true; opacity: 0.8; font.pixelSize: 12 }
                ScrollView {
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    clip: true
                    ListView {
                        id: fileList
                        model: openFiles
                        delegate: ItemDelegate {
                            width: fileList.width
                            text: modelData
                            elide: Text.ElideMiddle
                            onClicked: primo.loadFile(modelData)
                        }
                    }
                }
                Label { text: "Perf: QSG TextNode + visible cull"; font.pixelSize: 10; opacity: 0.5; wrapMode: Text.Wrap }
            }
        }

        ColumnLayout {
            SplitView.fillWidth: true
            spacing: 0

            TabBar {
                id: tabBar
                Layout.fillWidth: true
                Repeater {
                    model: openFiles.length ? openFiles : ["Untitled"]
                    TabButton { text: modelData.split("/").pop(); width: implicitWidth + 24; onClicked: primo.loadFile(modelData) }
                }
            }

            // Large file read-only banner
            Pane {
                id: largeBanner
                Layout.fillWidth: true
                visible: primo.readOnly && primo.document && primo.document.length > 50*1024*1024
                padding: 6
                background: Rectangle { color: "#ffcc00"; opacity: 0.9 }
                RowLayout {
                    anchors.fill: parent
                    Label { text: "⚠ Large file >50MB — read-only, async highlight only (visible + 200 lines)"; color: "black"; font.pixelSize: 11; Layout.fillWidth: true }
                    ToolButton { text: "Force Edit"; onClicked: primo.readOnly = false }
                }
            }

            // High-perf QSG editor + minimap + gutter
            RowLayout {
                Layout.fillWidth: true
                Layout.fillHeight: true
                spacing: 0

                Flickable {
                    id: flick
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    clip: true
                    contentHeight: primo.contentHeight
                    contentWidth: width
                    boundsBehavior: Flickable.StopAtBounds
                    ScrollBar.vertical: ScrollBar { policy: ScrollBar.AlwaysOn }
                    ScrollBar.horizontal: ScrollBar { policy: ScrollBar.AsNeeded }

                    PrimoEditor {
                        id: primo
                        width: flick.width
                        height: contentHeight
                        focus: true
                        gutterVisible: true
                        relativeNumbers: false
                        // Theme-aware colors (reuse palette, can be driven by SettingsManager later)
                        backgroundColor: palette.base
                        foregroundColor: palette.text
                        font.family: "Monospace"
                        font.pixelSize: 13
                        // Keep focus on click
                        Component.onCompleted: {
                            forceActiveFocus();
                            if (openFiles.length) loadFile(openFiles[0])
                        }
                        onFileLoaded: (ok, error) => {
                            if (!ok) console.warn("load failed:", error)
                            else flick.contentY = 0
                        }
                        // gutter bookmark toggle via Ctrl+B / gutter click already handled in C++
                        // Minimap sync diagnostics: example hook
                        onDiagnosticsChanged: minimap.requestPaint()
                        onBookmarksChanged: minimap.requestPaint()
                    }
                }

                // Minimap Canvas MVP (QSG still, but Canvas for simplicity)
                Minimap {
                    id: minimap
                    visible: primo.minimapVisible
                    Layout.preferredWidth: 100
                    Layout.fillHeight: true
                    editor: primo
                    flick: flick
                    bg: palette.base
                }
            }
            RowLayout {
                Layout.fillWidth: true
                spacing: 8
                CheckBox { text: "Gutter"; checked: primo.gutterVisible; onToggled: primo.gutterVisible = checked }
                CheckBox { text: "Rel. numbers"; checked: primo.relativeNumbers; onToggled: primo.relativeNumbers = checked }
                CheckBox { text: "Minimap"; checked: primo.minimapVisible; onToggled: primo.minimapVisible = checked }
                CheckBox { text: "Read-only"; checked: primo.readOnly; onToggled: primo.readOnly = checked }
                Label { text: "F2/Ctrl+B toggle bookmark"; opacity: 0.5; font.pixelSize: 10 }
            }

            Pane {
                Layout.fillWidth: true
                padding: 4
                background: Rectangle { color: palette.alternateBase }
                RowLayout {
                    anchors.fill: parent
                    Label { text: primo.filePath ? primo.filePath : "Untitled"; elide: Text.ElideMiddle; Layout.fillWidth: true; font.pixelSize: 11; opacity: 0.8 }
                    Label { text: primo.language ? primo.language : "Plain Text"; font.pixelSize: 11; opacity: 0.6 }
                    Label { text: primo.document ? primo.document.length + " chars • " + primo.document.lineCount + " lines" : "0 chars"; font.pixelSize: 11; opacity: 0.6 }
                }
            }
        }
    }

    Shortcut { sequence: "Ctrl+S"; onActivated: primo.save() }
    Shortcut { sequence: "Ctrl+O"; onActivated: console.log("Open dialog placeholder") }
}
