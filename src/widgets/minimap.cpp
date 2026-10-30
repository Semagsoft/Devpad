/*
 * Devpad - A C++/Qt6 code editor
 * Copyright (C) 2026 Semagsoft
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */
#include "minimap.h"

#include "codeeditor.h"

#include <QFontMetrics>
#include <QMouseEvent>
#include <QPainter>

namespace
{
// How many editor lines separate two sampled minimap rows. Painting every
// line is unnecessary and expensive on large files.
constexpr int SAMPLE_STEP = 4;
// Minimum height (in minimap pixels) of a sampled row so very short lines
// remain visible.
constexpr qreal MIN_ROW_HEIGHT = 1.0;
} // namespace

Minimap::Minimap(CodeEditor* editor, QWidget* parent) : QWidget(parent), m_editor(editor)
{
    setFixedWidth(m_stripWidth);
    setAttribute(Qt::WA_StaticContents, false);

    m_scrollPoll.setInterval(250);
    m_scrollPoll.setSingleShot(false);
    connect(&m_scrollPoll, &QTimer::timeout, this, &Minimap::onScrollPoll);

    updateEditorConnections();
    if (m_editor)
        m_scrollPoll.start();
}

void Minimap::setStripWidth(int width)
{
    if (width < 40)
        width = 40;
    if (m_stripWidth == width)
        return;
    m_stripWidth = width;
    setFixedWidth(width);
    update();
}

void Minimap::updateEditorConnections()
{
    if (!m_editor)
        return;

    disconnect(m_editor, nullptr, this, nullptr);

    connect(m_editor, &CodeEditor::textChanged, this, &Minimap::requestPaint);
    connect(m_editor, &CodeEditor::cursorPositionChanged, this, &Minimap::requestPaint);
    connect(m_editor, &CodeEditor::bookmarksChanged, this, &Minimap::requestPaint);
    connect(m_editor, &CodeEditor::diagnosticsChanged, this,
            [this](const QString&, const QList<lsp::Diagnostic>& diagnostics)
            {
                m_diagLines.clear();
                for (const auto& d : diagnostics)
                {
                    if (d.range.start.line >= 0)
                        m_diagLines.insert(d.range.start.line);
                    if (d.range.end.line >= 0 && d.range.end.line != d.range.start.line)
                        m_diagLines.insert(d.range.end.line);
                }
                requestPaint();
            });
}

void Minimap::requestPaint()
{
    update();
}

void Minimap::onScrollPoll()
{
    if (!m_editor)
        return;
    int first = m_editor->firstVisibleLine();
    if (first != m_lastFirstVisibleLine)
    {
        m_lastFirstVisibleLine = first;
        update();
    }
}

void Minimap::paintEvent(QPaintEvent* /*event*/)
{
    if (!m_editor)
        return;

    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing, false);

    const int w = width();
    const int h = height();
    if (w <= 0 || h <= 0)
        return;

    // Background: match editor palette so the strip blends in.
    p.fillRect(rect(), m_editor->palette().base());

    const int lineCount = m_editor->lines();
    if (lineCount <= 0)
        return;

    QFontMetrics fm(m_editor->font());
    const qreal lineHeight = qMax(static_cast<qreal>(fm.height()), 1.0);

    // Approximate total content height (no-wrap assumption).
    const qreal contentHeight = lineCount * lineHeight;
    const qreal scale = contentHeight > 0 ? h / contentHeight : 1.0;

    // Current viewport (what the editor is showing).
    const int firstVisible = m_editor->firstVisibleLine();
    m_lastFirstVisibleLine = firstVisible;
    const int viewportLines = qMax(1, m_editor->viewport()->height() / qMax(1, fm.height()));
    const qreal viewportTop = firstVisible * lineHeight * scale;
    const qreal viewportHeight = qMax(4.0, viewportLines * lineHeight * scale);

    // Viewport indicator.
    p.setOpacity(0.25);
    p.fillRect(0, viewportTop, w, viewportHeight, m_editor->palette().highlight());
    p.setOpacity(1.0);
    p.setPen(m_editor->palette().highlight().color());
    p.drawRect(0, viewportTop, w - 1, viewportHeight - 1);

    // Current line marker.
    int cursorLine = 0, cursorCol = 0;
    m_editor->getCursorPosition(&cursorLine, &cursorCol);
    const qreal curY = cursorLine * lineHeight * scale;
    p.setOpacity(0.10);
    p.fillRect(0, curY, w, lineHeight * scale, m_editor->palette().text().color());
    p.setOpacity(1.0);

    // Sampled line rows.
    const int step = SAMPLE_STEP;
    QSet<int> bookmarks;
    if (m_editor)
    {
        const QList<int> bm = m_editor->bookmarkLines();
        for (int l : bm)
            bookmarks.insert(l);
    }

    for (int i = 0; i < lineCount; i += step)
    {
        const qreal y = i * lineHeight * scale;
        if (y > h)
            break;
        const qreal rowHeight = qMax(MIN_ROW_HEIGHT, lineHeight * scale);
        if (y + rowHeight < 0)
            continue;

        QString text = m_editor->text(i);
        bool isBookmark = bookmarks.contains(i);
        bool isDiag = m_diagLines.contains(i);

        if (isBookmark)
        {
            p.setOpacity(0.9);
            p.fillRect(0, y, w, rowHeight, QColor(255, 160, 0)); // orange
            p.setOpacity(1.0);
            continue;
        }
        if (isDiag)
        {
            p.setOpacity(0.85);
            p.fillRect(0, y, w, rowHeight, QColor(255, 60, 60)); // red
            p.setOpacity(1.0);
            continue;
        }

        QColor color = colorForLine(text);
        p.setOpacity(0.5);
        const int rowW = qMin(w - 2, qMax(2, text.length() * 4)); // width ~ proportional to line length
        p.fillRect(0, y, rowW, rowHeight, color);
        p.setOpacity(1.0);
    }
}

void Minimap::resizeEvent(QResizeEvent* /*event*/)
{
    update();
}

QColor Minimap::colorForLine(const QString& text) const
{
    const QString trimmed = text.trimmed();
    if (trimmed.isEmpty())
        return palette().text().color();

    // Comment-ish lines.
    if (trimmed.startsWith(QLatin1String("//")) || trimmed.startsWith(QLatin1String("#")) || trimmed.startsWith(QLatin1String("/*")) ||
        trimmed.startsWith(QLatin1String("*")) || trimmed.startsWith(QLatin1String("'")) || trimmed.startsWith(QLatin1String("--")))
    {
        return QColor(140, 140, 140); // gray
    }
    // Preprocessor / directives.
    if (trimmed.startsWith(QLatin1String("#")))
        return QColor(140, 140, 140);

    const QStringView tv = QStringView(trimmed);
    // Keyword / definition starts.
    if (tv.startsWith(QLatin1String("function")) || tv.startsWith(QLatin1String("def ")) || tv.startsWith(QLatin1String("class ")) ||
        tv.startsWith(QLatin1String("struct ")) || tv.startsWith(QLatin1String("interface")) || tv.startsWith(QLatin1String("enum ")) ||
        tv.startsWith(QLatin1String("namespace")) || tv.startsWith(QLatin1String("pub ")) || tv.startsWith(QLatin1String("fn ")))
    {
        return QColor(180, 110, 220); // purple
    }
    // Imports / includes.
    if (tv.startsWith(QLatin1String("import")) || tv.startsWith(QLatin1String("include")) || tv.startsWith(QLatin1String("from ")) ||
        tv.startsWith(QLatin1String("using ")) || tv.startsWith(QLatin1String("require")))
    {
        return QColor(180, 110, 220); // purple
    }
    // Flow control.
    if (tv.startsWith(QLatin1String("if")) || tv.startsWith(QLatin1String("for")) || tv.startsWith(QLatin1String("while")) ||
        tv.startsWith(QLatin1String("switch")) || tv.startsWith(QLatin1String("case")) || tv.startsWith(QLatin1String("return")) ||
        tv.startsWith(QLatin1String("break")) || tv.startsWith(QLatin1String("continue")))
    {
        return QColor(180, 110, 220); // purple
    }
    // Variable declarations.
    if (tv.startsWith(QLatin1String("const")) || tv.startsWith(QLatin1String("let")) || tv.startsWith(QLatin1String("var")) ||
        tv.startsWith(QLatin1String("int ")) || tv.startsWith(QLatin1String("float ")) || tv.startsWith(QLatin1String("double ")) ||
        tv.startsWith(QLatin1String("bool ")) || tv.startsWith(QLatin1String("auto ")) || tv.startsWith(QLatin1String("string")))
    {
        return QColor(220, 180, 60); // yellow
    }
    // String literals present on the line.
    if (text.contains(QLatin1Char('"')) || text.contains(QLatin1Char('\'')) || text.contains(QLatin1String("`")))
    {
        return QColor(120, 180, 100); // green
    }
    // Numbers.
    bool ok = false;
    trimmed.toDouble(&ok);
    if (ok && !trimmed.contains(QLatin1Char('.')) && trimmed.length() < 8)
        return QColor(100, 180, 220); // blue

    return palette().text().color(); // regular code
}

void Minimap::mousePressEvent(QMouseEvent* event)
{
    if (event->button() != Qt::LeftButton || !m_editor)
    {
        QWidget::mousePressEvent(event);
        return;
    }
    scrollTo(event->position().y());
    event->accept();
}

void Minimap::mouseMoveEvent(QMouseEvent* event)
{
    if (event->buttons() & Qt::LeftButton && m_editor)
    {
        scrollTo(event->position().y());
        event->accept();
        return;
    }
    QWidget::mouseMoveEvent(event);
}

void Minimap::scrollTo(qreal y)
{
    if (!m_editor)
        return;

    const int lineCount = m_editor->lines();
    if (lineCount <= 0)
        return;

    QFontMetrics fm(m_editor->font());
    const qreal lineHeight = qMax(static_cast<qreal>(fm.height()), 1.0);
    const qreal contentHeight = lineCount * lineHeight;
    if (contentHeight <= 0)
        return;

    const qreal scale = height() / contentHeight;
    int line = qRound(y / scale / lineHeight);
    if (line < 0)
        line = 0;
    if (line >= lineCount)
        line = lineCount - 1;

    m_editor->setCursorPosition(line, 0);
    m_editor->ensureCursorVisible();
}
