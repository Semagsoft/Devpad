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
            Label { text: "primoEditor • QML"; opacity: 0.6; font.pixelSize: 11 }
            Item { Layout.fillWidth: true }
            Label { text: openFiles.length ? openFiles.length + " file(s)" : "No files"; opacity: 0.7 }
            ToolButton { text: "Open"; onClicked: fileDialog.open() }
        }
    }

    SplitView {
        anchors.fill: parent
        orientation: Qt.Horizontal

        // File / folder pane
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
            }
        }

        // Editor area
        ColumnLayout {
            SplitView.fillWidth: true
            spacing: 0

            TabBar {
                id: tabBar
                Layout.fillWidth: true
                Repeater {
                    model: openFiles.length ? openFiles : ["Untitled"]
                    TabButton { text: modelData.split("/").pop(); width: implicitWidth + 24 }
                }
            }

            // primoEditor host – for MVP a TextArea backed by PrimoEditor document
            // High-perf path will replace TextArea with custom QQuickItem rendering.
            ScrollView {
                Layout.fillWidth: true
                Layout.fillHeight: true
                clip: true
                TextArea {
                    id: editorArea
                    selectByMouse: true
                    persistentSelection: true
                    font.family: "Monospace"
                    font.pixelSize: 13
                    wrapMode: TextArea.NoWrap
                    placeholderText: "primoEditor — start typing…"
                    text: primo.text
                    onTextChanged: if (primo.text !== text) primo.text = text
                }
            }

            // Status bar
            Pane {
                Layout.fillWidth: true
                padding: 4
                background: Rectangle { color: palette.alternateBase }
                RowLayout {
                    anchors.fill: parent
                    Label { text: primo.filePath ? primo.filePath : "Untitled"; elide: Text.ElideMiddle; Layout.fillWidth: true; font.pixelSize: 11; opacity: 0.8 }
                    Label { text: primo.language ? primo.language : "Plain Text"; font.pixelSize: 11; opacity: 0.6 }
                    Label { text: editorArea.length + " chars"; font.pixelSize: 11; opacity: 0.6 }
                }
            }
        }
    }

    PrimoEditor {
        id: primo
        onFileLoaded: (ok, error) => {
            if (ok) editorArea.text = primo.text
            else console.warn("load failed:", error)
        }
        Component.onCompleted: {
            if (openFiles.length) loadFile(openFiles[0])
        }
    }

    // Minimal file dialog helper (QtQuick.Dialogs may not be available on all builds)
    // For now, placeholder – real dialog wired when Qt6 Dialogs module present.
}
