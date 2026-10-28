import QtQuick
import QtQuick.Controls 2.15
import QtQuick.Layouts
import QtQuick.Dialogs 6.4
import Devpad.Nextgen 1.0

// Full Options dialog for NextGen, mirrors Widgets OptionsDialog.
// All settings flow through NextgenActions <-> SettingsManager.
// Qt6 does not provide TabView (it was a Qt.labs.controls type), so the
// tabbed view is built with TabBar + StackLayout instead.
Dialog {
    id: root
    property var actions: null
    modal: true
    standardButtons: Dialog.Close
    width: 700
    height: 600
    title: qsTr("Options")

    Item {
        anchors.fill: parent
        ColumnLayout {
            TabBar {
                id: tabBar
                Layout.fillWidth: true
                currentIndex: 0
                TabButton { text: qsTr("General") }
                TabButton { text: qsTr("Editor") }
                TabButton { text: qsTr("Theme") }
                TabButton { text: qsTr("UI") }
                TabButton { text: qsTr("External Tools") }
            }
            Item {
                Layout.fillWidth: true
                Layout.fillHeight: true
                StackLayout {
                    currentIndex: tabBar.currentIndex

                    // ── General ──
                    ScrollView {
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        clip: true
                        ColumnLayout {
                            anchors.margins: 12
                            spacing: 16

                            GroupBox {
                                title: qsTr("Startup")
                                Layout.fillWidth: true
                                ColumnLayout {
                                    anchors.fill: parent
                                    spacing: 10
                                    RowLayout {
                                        Label { text: qsTr("Startup mode:"); Layout.preferredWidth: 140 }
                                        ComboBox {
                                            Layout.fillWidth: true
                                            model: [qsTr("New File"), qsTr("Restore Session"), qsTr("Open Last File"), qsTr("Empty")]
                                            currentIndex: 0 // TODO: bind to SettingsManager.startupMode()
                                            onActivated: (i) => { /* actions.setStartupMode(i) */ }
                                        }
                                    }
                                    RowLayout {
                                        CheckBox {
                                            text: qsTr("Check for updates on startup")
                                            checked: actions ? actions.checkForUpdatesEnabled : false
                                            onToggled: actions.setCheckForUpdatesEnabled(checked)
                                            Layout.fillWidth: true
                                        }
                                    }
                                }
                            }

                            GroupBox {
                                title: qsTr("Interface Font")
                                Layout.fillWidth: true
                                ColumnLayout {
                                    anchors.fill: parent
                                    spacing: 10
                                    RowLayout {
                                        Label { text: qsTr("Family:"); Layout.preferredWidth: 140 }
                                        TextField {
                                            Layout.fillWidth: true
                                            text: "Sans Serif" // TODO: bind to SettingsManager.uiFontFamily()
                                            onAccepted: actions.setDefaultFontFamily(text)
                                        }
                                    }
                                    RowLayout {
                                        Label { text: qsTr("Size:"); Layout.preferredWidth: 140 }
                                        SpinBox {
                                            from: 6; to: 72; value: 10
                                            Layout.fillWidth: true
                                            onValueChanged: actions.setDefaultFontSize(value)
                                        }
                                    }
                                }
                            }

                            GroupBox {
                                title: qsTr("Auto-save")
                                Layout.fillWidth: true
                                ColumnLayout {
                                    anchors.fill: parent
                                    spacing: 10
                                    RowLayout {
                                        CheckBox {
                                            text: qsTr("Enable auto-save")
                                            checked: actions ? actions.autoSaveEnabled : false
                                            onToggled: actions.setAutoSaveEnabled(checked)
                                            Layout.fillWidth: true
                                        }
                                    }
                                    RowLayout {
                                        Label { text: qsTr("Interval (seconds):"); Layout.preferredWidth: 140; enabled: actions ? actions.autoSaveEnabled : true }
                                        SpinBox {
                                            from: 10; to: 3600; value: 60
                                            enabled: actions ? actions.autoSaveEnabled : true
                                            Layout.fillWidth: true
                                            onValueChanged: actions.setAutoSaveInterval(value)
                                        }
                                    }
                                }
                            }
                        }
                    }

                    // ── Editor ──
                    ScrollView {
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        clip: true
                        ColumnLayout {
                            anchors.margins: 12
                            spacing: 16

                            GroupBox {
                                title: qsTr("Behavior")
                                Layout.fillWidth: true
                                ColumnLayout {
                                    anchors.fill: parent
                                    spacing: 10
                                    RowLayout {
                                        CheckBox { text: qsTr("Word wrap"); checked: actions ? actions.wordWrap : false
                                              onToggled: actions.setWordWrap(checked); Layout.fillWidth: true }
                                    }
                                    RowLayout {
                                        CheckBox { text: qsTr("Auto-close brackets"); checked: actions ? actions.autoCloseBrackets : false
                                              onToggled: actions.setAutoCloseBrackets(checked); Layout.fillWidth: true }
                                    }
                                    RowLayout {
                                        CheckBox { text: qsTr("Enable snippets"); checked: actions ? actions.snippetsEnabled : true
                                              onToggled: actions.setSnippetsEnabled(checked); Layout.fillWidth: true }
                                    }
                                    RowLayout {
                                        CheckBox { text: qsTr("Predictive snippets"); checked: actions ? actions.predictiveSnippets : true
                                              onToggled: actions.setPredictiveSnippets(checked); Layout.fillWidth: true }
                                    }
                                    RowLayout {
                                        CheckBox { text: qsTr("Auto-completion"); checked: actions ? actions.autoCompletionEnabled : true
                                              onToggled: actions.setAutoCompletionEnabled(checked); Layout.fillWidth: true }
                                    }
                                }
                            }

                            GroupBox {
                                title: qsTr("Formatting")
                                Layout.fillWidth: true
                                ColumnLayout {
                                    anchors.fill: parent
                                    spacing: 10
                                    RowLayout {
                                        Label { text: qsTr("Tab width:"); Layout.preferredWidth: 140 }
                                        SpinBox {
                                            from: 1; to: 16
                                            Layout.fillWidth: true
                                            Component.onCompleted: { value = actions ? actions.tabWidth : 4 }
                                            onValueChanged: { if (actions && value !== actions.tabWidth) actions.setTabWidth(value) }
                                        }
                                    }
                                }
                            }

                            GroupBox {
                                title: qsTr("Cursor")
                                Layout.fillWidth: true
                                ColumnLayout {
                                    anchors.fill: parent
                                    spacing: 10
                                    RowLayout {
                                        Label { text: qsTr("Style:"); Layout.preferredWidth: 140 }
                                        ComboBox {
                                            Layout.fillWidth: true
                                            model: [qsTr("Line"), qsTr("Block"), qsTr("Underline")]
                                            currentIndex: 0
                                            onActivated: (i) => { /* actions.setCursorStyle(i) */ }
                                        }
                                    }
                                    RowLayout {
                                        CheckBox { text: qsTr("Cursor blinking"); checked: true; Layout.fillWidth: true }
                                    }
                                    RowLayout {
                                        CheckBox { text: qsTr("Highlight current line"); checked: true; Layout.fillWidth: true }
                                    }
                                }
                            }
                        }
                    }

                    // ── Theme ──
                    ScrollView {
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        clip: true
                        ColumnLayout {
                            anchors.margins: 12
                            spacing: 16

                            GroupBox {
                                title: qsTr("Theme selection")
                                Layout.fillWidth: true
                                ColumnLayout {
                                    anchors.fill: parent
                                    spacing: 10
                                    RowLayout {
                                        Label { text: qsTr("Theme:"); Layout.preferredWidth: 140 }
                                        ComboBox {
                                            id: themeCombo
                                            Layout.fillWidth: true
                                            model: actions ? actions.themeNames : []
                                            currentIndex: {
                                                if (!actions || !themeCombo.model) return 0;
                                                var cur = actions.currentThemeName;
                                                var idx = themeCombo.model.indexOf(cur);
                                                return idx >= 0 ? idx : 0;
                                            }
                                            onActivated: (idx) => {
                                                if (!themeCombo.model || idx < 0 || idx >= themeCombo.model.length) return;
                                                actions.setThemeByName(themeCombo.model[idx]);
                                            }
                                        }
                                    }
                                    RowLayout {
                                        Label { text: qsTr("Current:"); Layout.preferredWidth: 140 }
                                        Label { text: (actions && actions.currentThemeName) ? actions.currentThemeName : qsTr("Light"); Layout.fillWidth: true }
                                    }
                                }
                            }

                            GroupBox {
                                title: qsTr("Accent color")
                                Layout.fillWidth: true
                                ColumnLayout {
                                    anchors.fill: parent
                                    spacing: 10
                                    RowLayout {
                                        Rectangle {
                                            id: accentSwatch
                                            width: 24; height: 24
                                            color: "#3399ff"
                                            border.color: "black"
                                            radius: 4
                                        }
                                        ToolButton {
                                            text: qsTr("Choose...")
                                            onClicked: colorDialog.open()
                                        }
                                        ToolButton {
                                            text: qsTr("Clear")
                                            onClicked: { /* actions.clearAccentColor() */ }
                                        }
                                        Label { text: qsTr("Accent color applied to selection, cursor, brace matching"); Layout.fillWidth: true; opacity: 0.7; wrapMode: Text.Wrap }
                                    }
                                }
                            }

                            ColorDialog {
                                id: colorDialog
                                title: qsTr("Choose Accent Color")
                                onAccepted: {
                                    // actions.setAccentColor(colorDialog.color)
                                    accentSwatch.color = colorDialog.color
                                }
                            }
                        }
                    }

                    // ── UI ──
                    ScrollView {
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        clip: true
                        ColumnLayout {
                            anchors.margins: 12
                            spacing: 16

                            GroupBox {
                                title: qsTr("Visibility")
                                Layout.fillWidth: true
                                ColumnLayout {
                                    anchors.fill: parent
                                    spacing: 10
                                    RowLayout {
                                        CheckBox { text: qsTr("Show menu bar"); checked: actions ? actions.showMenuBar : true
                                              onToggled: actions.setShowMenuBar(checked); Layout.fillWidth: true }
                                    }
                                    RowLayout {
                                        CheckBox { text: qsTr("Show tool bar"); checked: actions ? actions.showToolbar : true
                                              onToggled: actions.setShowToolbar(checked); Layout.fillWidth: true }
                                    }
                                    RowLayout {
                                        CheckBox { text: qsTr("Show status bar"); checked: actions ? actions.showStatusbar : true
                                              onToggled: actions.setShowStatusbar(checked); Layout.fillWidth: true }
                                    }
                                }
                            }

                            GroupBox {
                                title: qsTr("Project")
                                Layout.fillWidth: true
                                ColumnLayout {
                                    anchors.fill: parent
                                    spacing: 10
                                    RowLayout {
                                        CheckBox { text: qsTr("Show hidden files"); checked: actions ? actions.showHiddenFiles : false
                                              onToggled: actions.setShowHiddenFiles(checked); Layout.fillWidth: true }
                                    }
                                    RowLayout {
                                        CheckBox { text: qsTr("Use .gitignore filtering"); checked: actions ? actions.useGitIgnore : true
                                              onToggled: actions.setUseGitIgnore(checked); Layout.fillWidth: true }
                                    }
                                }
                            }
                        }
                    }

                    // ── External Tools ──
                    ScrollView {
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        clip: true
                        ColumnLayout {
                            anchors.margins: 12
                            spacing: 16

                            GroupBox {
                                title: qsTr("Configured tools")
                                Layout.fillWidth: true
                                ColumnLayout {
                                    anchors.fill: parent
                                    spacing: 10

                                    ListView {
                                        id: toolList
                                        Layout.fillWidth: true
                                        Layout.fillHeight: true
                                        model: actions.externalToolCount
                                        delegate: ItemDelegate {
                                            width: toolList.width
                                            text: modelData
                                            RowLayout {
                                                anchors.fill: parent
                                                Label { text: actions.externalToolName(index); Layout.fillWidth: true }
                                                Label { text: actions.externalToolCommand(index); font.pixelSize: 10; opacity: 0.7 }
                                                ToolButton {
                                                    text: qsTr("Remove")
                                                    onClicked: actions.removeExternalTool(index)
                                                }
                                            }
                                        }
                                    }

                                    RowLayout {
                                        Layout.fillWidth: true
                                        spacing: 8
                                        TextField {
                                            id: toolNameField
                                            Layout.fillWidth: true
                                            placeholderText: qsTr("Tool name")
                                        }
                                        TextField {
                                            id: toolCmdField
                                            Layout.fillWidth: true
                                            placeholderText: qsTr("Command")
                                        }
                                        ToolButton {
                                            text: qsTr("Add")
                                            onClicked: {
                                                if (toolNameField.text && toolCmdField.text)
                                                    actions.addExternalTool(toolNameField.text, toolCmdField.text, "", "", "", false);
                                                toolNameField.text = "";
                                                toolCmdField.text = "";
                                            }
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
    }
}
