import QtQuick 2.15

Item {
    id: root
    property var editor: null
    property var flick: null
    property color bg: "#1e1e2e"
    property real scale: editor ? height / (editor.contentHeight || 1) : 1

    width: 100
    clip: true

    Rectangle {
        anchors.fill: parent
        color: bg
        opacity: 0.9
        border.color: Qt.rgba(0.5,0.5,0.5,0.2)
        border.width: 1
    }

    Canvas {
        id: canvas
        anchors.fill: parent
        anchors.margins: 2
        onPaint: {
            if (!editor || !editor.document) return;
            var ctx = getContext("2d");
            ctx.reset();
            ctx.clearRect(0,0,width,height);
            var doc = editor.document;
            var lineCount = doc.lineCount;
            if (lineCount <= 0) return;
            var totalH = editor.contentHeight || (lineCount * 18);
            var s = height / totalH;
            // Sample step to avoid drawing 100k lines individually: batch
            var step = Math.max(1, Math.floor(lineCount / height));
            var maxChars = 80;
            // Get bookmarks / diagnostics via editor
            var bms = editor.bookmarkLines ? editor.bookmarkLines() : [];
            var bset = {};
            for (var bi=0; bi<bms.length; ++bi) bset[bms[bi]] = true;
            // diagnostics: assume editor has diagLines property? For now check via editor.diagnosticsChanged?
            // Use editor._diagSet if available; fallback empty
            var diagSet = root.diagSet || {};

            for (var i=0; i<lineCount; i+= step) {
                var y = i * editor.lineHeight * s;
                var line = doc.lineAt(i);
                var w = Math.min(width-4, (line.length / maxChars) * (width-4));
                if (w < 2) w = 2;
                if (bset[i]) {
                    ctx.fillStyle = "#ffa500"; // bookmark orange
                    ctx.fillRect(0, y, width-2, 2);
                } else if (diagSet[i]) {
                    ctx.fillStyle = "#ff0000";
                    ctx.fillRect(0, y, width-2, 2);
                } else {
                    // Use foreground with alpha based on content? simple gray
                    var isComment = line.trim().startsWith("//") || line.trim().startsWith("#");
                    if (isComment) ctx.fillStyle = "#6c7086";
                    else if (line.trim().length === 0) continue;
                    else ctx.fillStyle = "#89b4fa";
                    ctx.globalAlpha = 0.55;
                    ctx.fillRect(0, y, w, 1.2);
                    ctx.globalAlpha = 1.0;
                }
            }
            // viewport indicator
            if (flick) {
                var vy = flick.contentY * s;
                var vh = flick.height * s;
                ctx.fillStyle = "rgba(180,180,180,0.18)";
                ctx.fillRect(0, vy, width, Math.max(4, vh));
                ctx.strokeStyle = "rgba(200,200,200,0.35)";
                ctx.lineWidth = 1;
                ctx.strokeRect(0, vy, width, Math.max(4, vh));
            }
        }
        // repaint on scroll / text change
        Connections {
            target: editor
            function onTextChanged() { canvas.requestPaint(); }
            function onBookmarksChanged() { canvas.requestPaint(); }
            function onDiagnosticsChanged() { canvas.requestPaint(); }
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
            // rebuild diagSet from editor (assume editor has method)
            // For now, try to query via editor._diagLines if exposed
            root.diagSet = {};
            // editor.diagnostics is QSet, not directly accessible; we use bookmarkLines as proxy
            // Next iteration will expose diagnostics lines as list
            canvas.requestPaint();
        }
    }

    MouseArea {
        anchors.fill: parent
        onClicked: (mouse) => {
            if (!editor || !flick) return;
            var totalH = editor.contentHeight || 1;
            var s = height / totalH;
            var targetY = mouse.y / s - flick.height/2;
            if (targetY < 0) targetY = 0;
            var maxY = flick.contentHeight - flick.height;
            if (targetY > maxY) targetY = maxY;
            flick.contentY = targetY;
        }
        onPositionChanged: (mouse) => {
            if (pressed) {
                var totalH = editor.contentHeight || 1;
                var s = height / totalH;
                var targetY = mouse.y / s - flick.height/2;
                if (targetY < 0) targetY = 0;
                var maxY = flick.contentHeight - flick.height;
                if (targetY > maxY) targetY = maxY;
                flick.contentY = targetY;
            }
        }
    }

    // auto repaint timer for async highlight
    Timer {
        interval: 400
        running: true
        repeat: true
        onTriggered: canvas.requestPaint()
    }
}
