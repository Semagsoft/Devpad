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

#ifndef MINIMAP_H
#define MINIMAP_H

#include "lsp/lsptypes.h"

#include <QSet>
#include <QTimer>
#include <QWidget>

class CodeEditor;

// Minimap: a thin vertical strip rendering a down-sampled thumbnail of the
// document next to a CodeEditor. Shows syntax-tinted line samples, a viewport
// indicator synced to the editor scroll position, bookmark and diagnostic
// markers, and supports click/drag to scroll the editor.
// Rendering is intentionally simple (per-line color sampling) so it stays
// cheap even on large files: only every Nth line is painted.
class Minimap : public QWidget
{
    Q_OBJECT

public:
    explicit Minimap(CodeEditor* editor, QWidget* parent = nullptr);
    ~Minimap() override = default;

    CodeEditor* editor() const
    {
        return m_editor;
    }

    // Width of the minimap strip in pixels.
    void setStripWidth(int width);
    int stripWidth() const
    {
        return m_stripWidth;
    }

protected:
    void paintEvent(QPaintEvent* event) override;
    void resizeEvent(QResizeEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;

private slots:
    void requestPaint();
    void onScrollPoll();

private:
    void updateEditorConnections();
    QColor colorForLine(const QString& text) const;
    void scrollTo(qreal y);

    CodeEditor* m_editor = nullptr;
    int m_stripWidth = 120;
    QTimer m_scrollPoll;
    int m_lastFirstVisibleLine = -1;
    QSet<int> m_diagLines;
};

#endif // MINIMAP_H
