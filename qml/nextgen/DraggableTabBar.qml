import QtQuick
import QtQuick.Controls

// Draggable TabBar between panes – icon not needed, just text + close
TabBar {
    id: root
    property var tabModel: null
    property int paneId: 0 // 0 = left, 1 = right
    property var editor: null // PrimoEditor for switching

    // Use tabModel.tabs as model
    Repeater {
        model: tabModel ? tabModel.tabs : []
        TabButton {
            id: tabBtn
            text: tabModel ? tabModel.displayName(index) : modelData
            width: Math.max(80, Math.min(200, implicitWidth + 30))
            // Close button (like closeButtonMode)
            contentItem: Row {
                spacing: 6
                Text {
                    text: tabBtn.text
                    elide: Text.ElideRight
                    anchors.verticalCenter: parent.verticalCenter
                    color: tabBtn.checked ? palette.highlightedText : palette.windowText
                }
                ToolButton {
                    text: "×"
                    font.pixelSize: 14
                    flat: true
                    padding: 2
                    onClicked: tabModel.closeTab(index)
                }
            }

            // Drag support
            DragHandler {
                id: dragHandler
                onActiveChanged: if (active) { root.draggedTabIndex = index; root.draggedPaneId = paneId }
            }
            Drag.active: dragHandler.active
            Drag.source: tabBtn
            Drag.hotSpot.x: width/2
            Drag.hotSpot.y: height/2

            // Visual feedback while dragging
            states: State {
                when: tabBtn.Drag.active
                ParentChange { target: tabBtn; parent: dragContainer }
                AnchorChanges { target: tabBtn; anchors.horizontalCenter: undefined; anchors.verticalCenter: undefined }
            }

            onClicked: {
                if (paneId === 0) tabModel.currentIndex = index
                else tabModel.pane2Index = index
                // Load file into associated editor
                var path = tabModel.tabAt(index)
                if (path && editor) editor.loadFile(path)
            }

            MouseArea {
                anchors.fill: parent
                acceptedButtons: Qt.MiddleButton | Qt.RightButton
                onClicked: (mouse) => {
                    if (mouse.button === Qt.MiddleButton) tabModel.closeTab(index)
                    else if (mouse.button === Qt.RightButton) contextMenu.popup()
                }
                Menu {
                    id: contextMenu
                    MenuItem { text: "Close"; onTriggered: tabModel.closeTab(index) }
                    MenuItem { text: "Detach to new window"; onTriggered: tabModel.detachTab(index) }
                    MenuItem { text: "Move to other pane"; onTriggered: tabModel.moveTabToPane(paneId, index, paneId===0?1:0, 0) }
                }
            }
        }
    }

    // Container for dragged item
    Item {
        id: dragContainer
        anchors.fill: parent
        visible: false
    }

    // DropArea for this pane – accepts drops from other pane
    DropArea {
        anchors.fill: parent
        onDropped: (drop) => {
            if (draggedTabIndex < 0) return;
            var pos = drop.x
            var tabW = 120 // approximate
            var toIdx = Math.floor(pos / tabW)
            if (toIdx < 0) toIdx = 0
            if (toIdx >= tabModel.count) toIdx = tabModel.count -1
            if (draggedPaneId === paneId) {
                tabModel.moveTab(draggedTabIndex, toIdx)
            } else {
                tabModel.moveTabToPane(draggedPaneId, draggedTabIndex, paneId, toIdx)
            }
            draggedTabIndex = -1
            draggedPaneId = -1
        }
    }

    property int draggedTabIndex: -1
    property int draggedPaneId: -1

    // Track drag start
    Component.onCompleted: {
        // Connect to drag start via children
    }
}
