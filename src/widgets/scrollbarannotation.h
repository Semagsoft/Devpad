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

#ifndef SCROLLBARANNOTATION_H
#define SCROLLBARANNOTATION_H

#include <QList>
#include <QPair>
#include <QWidget>

/**
 * Thin overlay strip meant to sit next to the vertical scrollbar and paint
 * proportional tick marks for bookmarks and LSP diagnostics. Mouse-transparent.
 * The host editor feeds it (line -> severity color) data; it knows nothing
 * about Scintilla or the LSP layer.
 */
class ScrollbarAnnotation : public QWidget
{
    Q_OBJECT

public:
    explicit ScrollbarAnnotation(QWidget* parent = nullptr);

    /** Replace all annotation data. Line numbers are 0-based. */
    void setAnnotations(const QList<QPair<int, QColor>>& ticks, int totalLines);

protected:
    void paintEvent(QPaintEvent* event) override;

private:
    QList<QPair<int, QColor>> m_ticks; // (line, color); drawn in list order
    int m_totalLines = 1;
};

#endif // SCROLLBARANNOTATION_H
