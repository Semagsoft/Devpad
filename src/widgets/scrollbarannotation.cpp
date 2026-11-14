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

#include "scrollbarannotation.h"

#include <QPainter>
#include <QPaintEvent>

ScrollbarAnnotation::ScrollbarAnnotation(QWidget* parent) : QWidget(parent)
{
    setAttribute(Qt::WA_TransparentForMouseEvents);
    setAttribute(Qt::WA_TranslucentBackground);
    setFocusPolicy(Qt::NoFocus);
    setAttribute(Qt::WA_NoSystemBackground);
}

void ScrollbarAnnotation::setAnnotations(const QList<QPair<int, QColor>>& ticks, int totalLines)
{
    m_ticks = ticks;
    m_totalLines = totalLines > 0 ? totalLines : 1;
    update();
}

void ScrollbarAnnotation::paintEvent(QPaintEvent* event)
{
    Q_UNUSED(event);
    QPainter painter(this);
    const int h = height();
    if (h <= 0)
        return;
    const qreal scale = static_cast<qreal>(h - 4) / static_cast<qreal>(m_totalLines > 1 ? m_totalLines - 1 : 1);
    for (const auto& [line, color] : m_ticks)
    {
        if (line < 0)
            continue;
        const int y = qBound(0, static_cast<int>(2 + line * scale), h - 3);
        painter.fillRect(0, y, width(), 2, color);
    }
}
