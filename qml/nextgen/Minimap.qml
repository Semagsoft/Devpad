import QtQuick
import QtQuick.Controls

// Minimap: thumbnail overview of the document with syntax-aware coloring,
// viewport indicator, bookmark and diagnostic markers, and click-to-scroll.
Item {
    id: root
    property var editor: null
    property var flick: null
    property color bg: "#1e1e2e"
    property real scale: editor ? height / (editor.contentHeight || 1) : 1

    width: 100
    clip: true

    // Background
    Rectangle {
        anchors.fill: parent
        color: bg
        opacity: 0.9
        border.color: Qt.rgba(0.5, 0.5, 0.5, 0.2)
        border.width: 1
    }

    // Line number gutter (left side of minimap)
    Row {
        id: lineNumberGutter
        anchors.left: parent.left
        anchors.top: parent.top
        anchors.bottom: parent.bottom
        width: 14
        spacing: 0
        Repeater {
            model: editor ? editor.document.lineCount : 0
            delegate: Text {
                text: index + 1
                font.pixelSize: 7
                color: "#555"
                width: 14
                horizontalAlignment: Text.AlignRight
                visible: index % 5 === 0 // show every 5th line number to reduce clutter
                opacity: editor && index === editor.cursorLine ? 1.0 : 0.4
            }
        }
    }

    // Canvas: content + viewport + markers
    Canvas {
        id: canvas
        anchors.left: lineNumberGutter.right
        anchors.right: parent.right
        anchors.top: parent.top
        anchors.bottom: parent.bottom
        anchors.margins: 0
        onPaint: {
            if (!editor || !editor.document) return;
            var ctx = getContext("2d");
            ctx.reset();
            ctx.clearRect(0, 0, width, height);

            var doc = editor.document;
            var lineCount = doc.lineCount;
            if (lineCount <= 0) return;
            var totalH = editor.contentHeight || (lineCount * 18);
            var s = height / totalH;

            // Sample step to avoid drawing 100k lines individually
            var step = Math.max(1, Math.floor(lineCount / height));
            var maxChars = 120;

            // Get bookmarks / diagnostics via editor
            var bms = editor.bookmarkLines ? editor.bookmarkLines() : [];
            var bset = {};
            for (var bi = 0; bi < bms.length; ++bi) bset[bms[bi]] = true;

            // Diagnostics from editor
            var diagSet = root.diagSet || {};

            for (var i = 0; i < lineCount; i += step) {
                var y = i * editor.lineHeight * s;
                var line = doc.lineAt(i);
                var trimmed = line.trim();

                // Determine color based on line content
                var color;
                if (trimmed.length === 0) {
                    continue; // skip empty lines
                } else if (trimmed.startsWith("//") || trimmed.startsWith("#") ||
                           trimmed.startsWith("/*") || trimmed.startsWith("*")) {
                    color = "#6c7086"; // comment
                } else if (trimmed.startsWith("function") || trimmed.startsWith("def ") ||
                           trimmed.startsWith("class ") || trimmed.startsWith("struct ") ||
                           trimmed.startsWith("interface ") || trimmed.startsWith("enum ")) {
                    color = "#c678dd"; // keyword/purple
                } else if (trimmed.startsWith("import") || trimmed.startsWith("include") ||
                           trimmed.startsWith("from ")) {
                    color = "#c678dd";
                } else if (trimmed.startsWith("if") || trimmed.startsWith("for") ||
                           trimmed.startsWith("while") || trimmed.startsWith("switch") ||
                           trimmed.startsWith("case") || trimmed.startsWith("return")) {
                    color = "#c678dd";
                } else if (line.includes('"') || line.includes("'")) {
                    color = "#98c379"; // string/green
                } else if (trimmed.startsWith("const") || trimmed.startsWith("let") ||
                           trimmed.startsWith("var")) {
                    color = "#e5c07b"; // variable/yellow
                } else {
                    color = "#89b4fa"; // regular code/blue
                }

                var w = Math.min(width - 4, (line.length / maxChars) * (width - 4));
                if (w < 2) w = 2;

                if (bset[i]) {
                    ctx.fillStyle = "#ffa500"; // bookmark orange
                    ctx.fillRect(0, y, width - 2, 2);
                } else if (diagSet[i]) {
                    ctx.fillStyle = "#ff0000"; // diagnostic red
                    ctx.fillRect(0, y, width - 2, 2);
                } else {
                    ctx.fillStyle = color;
                    ctx.globalAlpha = 0.55;
                    ctx.fillRect(0, y, w, 1.2);
                    ctx.globalAlpha = 1.0;
                }
            }

            // Viewport indicator
            if (flick) {
                var vy = flick.contentY * s;
                var vh = flick.height * s;
                if (vh < 6) vh = 6;
                ctx.fillStyle = "rgba(180, 180, 180, 0.15)";
                ctx.fillRect(0, vy, width, Math.max(4, vh));
                ctx.strokeStyle = "rgba(220, 220, 220, 0.4)";
                ctx.lineWidth = 1;
                ctx.strokeRect(0, vy, width, Math.max(4, vh));
            }

            // Current line indicator
            if (editor && editor.cursorLine >= 0) {
                var cy = editor.cursorLine * editor.lineHeight * s;
                ctx.fillStyle = "rgba(255, 255, 255, 0.08)";
                ctx.fillRect(0, cy, width, editor.lineHeight * s);
            }
        }

        // Repaint on scroll / text change
        Connections {
            target: editor
            function onTextChanged() { canvas.requestPaint(); }
            function onBookmarksChanged() { canvas.requestPaint(); }
            function onDiagnosticsChanged() { canvas.requestPaint(); }
            function onCursorChanged() { canvas.requestPaint(); }
        }
        Connections {
            target: flick
            function onContentYChanged() { canvas.requestPaint(); }
        }
        Component.onCompleted: requestPaint()
        onWidthChanged: requestPaint()
        onHeightChanged: requestPaint()
    }

    function requestPaint() { canvas.requestPaint(); }
    property var diagSet: ({})
    Connections {
        target: editor
        function onDiagnosticsChanged() {
            root.diagSet = {};
            canvas.requestPaint();
        }
    }

    // Click / drag to scroll
    MouseArea {
        anchors.fill: parent
        onClicked: (mouse) => {
            if (!editor || !flick) return;
            var totalH = editor.contentHeight || 1;
            var s = height / totalH;
            var targetY = mouse.y / s - flick.height / 2;
            if (targetY < 0) targetY = 0;
            var maxY = flick.contentHeight - flick.height;
            if (targetY > maxY) targetY = maxY;
            flick.contentY = targetY;
        }
        onPositionChanged: (mouse) => {
            if (mouse.pressed) {
                var totalH = editor.contentHeight || 1;
                var s = height / totalH;
                var targetY = mouse.y / s - flick.height / 2;
                if (targetY < 0) targetY = 0;
                var maxY = flick.contentHeight - flick.height;
                if (targetY > maxY) targetY = maxY;
                flick.contentY = targetY;
            }
        }
    }

    // Auto repaint timer for async highlight (reduced frequency)
    Timer {
        interval: 800
        running: true
        repeat: true
        onTriggered: canvas.requestPaint()
    }
}
