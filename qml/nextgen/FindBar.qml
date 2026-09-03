import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

// Inline Find/Replace bar – replace in same bar, not separate dialog
Pane {
    id: root
    property var editor: null
    property bool replaceVisible: false
    property bool caseSensitive: false
    property bool wholeWord: false
    property bool useRegex: false

    visible: false
    padding: 4
    background: Rectangle { color: palette.alternateBase; border.color: palette.mid; border.width: 1; radius: 4 }

    function show(findOnly) {
        visible = true
        replaceVisible = !findOnly
        findField.forceActiveFocus()
        findField.selectAll()
    }
    function hideBar() { visible = false; if (editor) editor.forceActiveFocus(); }
    function doFind(next) {
        if (!editor || !editor.document) return
        var pat = findField.text
        if (!pat) return
        var txt = editor.document.text
        var flags = caseSensitive ? 0 : 1 // 1 = case-insensitive
        // Use JS search for MVP; for regex use RegExp
        var re
        try {
            if (useRegex) re = new RegExp(pat, (caseSensitive?"g":"gi"))
            else {
                var esc = pat.replace(/[.*+?^${}()|[\]\\]/g, '\\$&')
                var word = wholeWord ? "\\b" + esc + "\\b" : esc
                re = new RegExp(word, caseSensitive?"g":"gi")
            }
        } catch(e) { status.text = "Invalid regex"; return; }
        var pos = 0
        // Compute current offset from cursor
        var curLine = editor.cursorLine
        var curCol = editor.cursorColumn
        var curOffset = 0
        for (var i=0;i<curLine;i++) curOffset += editor.document.lineAt(i).length + 1
        curOffset += curCol
        var match
        var found = -1
        var foundLine = -1
        var foundCol = -1
        // Find all matches to count
        var all = []
        while ((match = re.exec(txt)) !== null) {
            all.push(match.index)
            if (match[0].length === 0) re.lastIndex++
            if (all.length > 5000) break // limit
        }
        status.text = all.length + " matches"
        if (all.length === 0) return
        // Find next/prev relative to curOffset
        if (next) {
            for (var k=0;k<all.length;k++) if (all[k] > curOffset) { found = all[k]; break; }
            if (found===-1) found = all[0]
        } else {
            for (var k=all.length-1;k>=0;k--) if (all[k] < curOffset) { found = all[k]; break; }
            if (found===-1) found = all[all.length-1]
        }
        // Convert offset to line/col
        var off=0, line=0, col=0
        for (var l=0; l<editor.document.lineCount; l++) {
            var len = editor.document.lineAt(l).length + 1
            if (found < off+len) { line=l; col=found-off; break; }
            off+=len
        }
        editor.setCursorPosition(line, col)
        // Scroll to center
        if (flick) flick.contentY = Math.max(0, line * editor.lineHeight - flick.height/2)
        // Highlight via diagnostics? For MVP just move cursor
    }
    function doReplace() {
        if (!editor || editor.readOnly || editor.isUndoDisabled) return
        var pat = findField.text
        var rep = replaceField.text
        if (!pat) return
        var txt = editor.document.text
        var flags = caseSensitive ? "g" : "gi"
        try {
            var re
            if (useRegex) re = new RegExp(pat, flags)
            else {
                var esc = pat.replace(/[.*+?^${}()|[\]\\]/g, '\\$&')
                var word = wholeWord ? "\\b" + esc + "\\b" : esc
                re = new RegExp(word, flags)
            }
            var newTxt = txt.replace(re, rep)
            if (newTxt !== txt) {
                editor.text = newTxt
                status.text = "Replaced"
            }
        } catch(e) { status.text = "Invalid regex" }
    }
    function doReplaceAll() {
        doReplace()
    }

    property var flick: null

    ColumnLayout {
        anchors.fill: parent
        spacing: 4
        RowLayout {
            Layout.fillWidth: true
            spacing: 6
            Label { text: "Find:" }
            TextField {
                id: findField
                Layout.fillWidth: true
                placeholderText: "Search..."
                onAccepted: doFind(true)
                onTextChanged: doFind(true)
                Keys.onEscapePressed: root.hideBar()
            }
            ToolButton {
                icon.source: "qrc:/icons/Edit/find.svg"
                ToolTip.text: "Find Next (Enter)"
                onClicked: doFind(true)
            }
            ToolButton {
                icon.source: "qrc:/icons/Edit/findprevious.svg"
                ToolTip.text: "Find Previous (Shift+Enter)"
                onClicked: doFind(false)
            }
            ToolButton {
                text: replaceVisible ? " − " : " + "
                ToolTip.text: replaceVisible ? "Hide Replace" : "Show Replace"
                onClicked: replaceVisible = !replaceVisible
            }
            ToolButton {
                icon.source: "qrc:/icons/File/close.svg"
                ToolTip.text: "Close (Esc)"
                onClicked: root.hideBar()
            }
        }
        RowLayout {
            id: replaceRow
            visible: replaceVisible
            Layout.fillWidth: true
            spacing: 6
            Label { text: "Replace:" }
            TextField {
                id: replaceField
                Layout.fillWidth: true
                placeholderText: "Replace with..."
                onAccepted: doReplace()
            }
            ToolButton { text: "Replace"; onClicked: doReplace() }
            ToolButton { text: "Replace All"; onClicked: doReplaceAll() }
        }
        RowLayout {
            Layout.fillWidth: true
            spacing: 8
            CheckBox { text: "Aa"; checked: root.caseSensitive; onToggled: root.caseSensitive = checked; ToolTip.text: "Case sensitive" }
            CheckBox { text: "W"; checked: root.wholeWord; onToggled: root.wholeWord = checked; ToolTip.text: "Whole word" }
            CheckBox { text: ".*"; checked: root.useRegex; onToggled: root.useRegex = checked; ToolTip.text: "Regex" }
            Label { id: status; Layout.fillWidth: true; opacity: 0.7; font.pixelSize: 11 }
        }
    }

    // Keyboard shortcuts within bar
    Keys.onPressed: (event) => {
        if (event.key === Qt.Key_Escape) { hideBar(); event.accepted = true }
        if (event.key === Qt.Key_F3) { doFind(!(event.modifiers & Qt.ShiftModifier)); event.accepted = true }
    }
}
