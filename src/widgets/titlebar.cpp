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

#include "titlebar.h"

#include "settingsmanager.h"
#include "theme.h"

#include <QHBoxLayout>
#include <QMenuBar>
#include <QMouseEvent>
#include <QPainter>

namespace
{
constexpr int WindowButtonWidth = 46;
constexpr int WindowButtonHeight = TitleBar::Height;
} // namespace

TitleBarButton::TitleBarButton(Role role, QWidget* parent) : QToolButton(parent), m_role(role)
{
    setObjectName("TitleBarWindowButton");
    setFocusPolicy(Qt::NoFocus);
    setAutoRaise(true);
    setFixedSize(WindowButtonWidth, WindowButtonHeight);
    setCursor(Qt::ArrowCursor);
}

void TitleBarButton::setMaximized(bool maximized)
{
    if (m_role == Role::Maximize)
        m_role = maximized ? Role::Restore : Role::Maximize;
    update();
}

void TitleBarButton::paintEvent(QPaintEvent* event)
{
    Q_UNUSED(event)

    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    bool closeHovered = (m_role == Role::Close) && (underMouse() || isDown());
    if (closeHovered)
    {
        painter.fillRect(rect(), QColor(0xE8, 0x11, 0x23));
        painter.setPen(QColor(255, 255, 255));
    }
    else
    {
        painter.setPen(palette().color(QPalette::WindowText));
    }

    const QColor lineColor = painter.pen().color();
    const QRect r = rect();
    const int penWidth = 1;
    painter.setPen(QPen(lineColor, penWidth));

    const QPointF center(r.center().x() + 0.5, r.center().y() + 0.5);

    switch (m_role)
    {
    case Role::Minimize:
        painter.drawLine(QPointF(center.x() - 7, center.y() + 0.5), QPointF(center.x() + 7, center.y() + 0.5));
        break;
    case Role::Maximize:
    {
        QRectF box(center.x() - 7, center.y() - 6, 14, 12);
        painter.drawRect(box);
        break;
    }
    case Role::Restore:
    {
        QRectF front(center.x() - 6, center.y() - 4, 12, 10);
        QRectF back(center.x() - 8, center.y() - 6, 12, 10);
        painter.drawRect(front);
        painter.drawRect(back);
        painter.fillRect(back.intersected(front), palette().window());
        break;
    }
    case Role::Close:
        painter.drawLine(QPointF(center.x() - 6, center.y() - 6), QPointF(center.x() + 6, center.y() + 6));
        painter.drawLine(QPointF(center.x() - 6, center.y() + 6), QPointF(center.x() + 6, center.y() - 6));
        break;
    }
}

TitleBar::TitleBar(QWidget* parent) : QWidget(parent)
{
    setObjectName("TitleBar");
    setFixedHeight(Height);

    m_minimizeButton = new TitleBarButton(TitleBarButton::Role::Minimize, this);
    m_minimizeButton->setToolTip(tr("Minimize"));
    m_maximizeButton = new TitleBarButton(TitleBarButton::Role::Maximize, this);
    m_maximizeButton->setToolTip(tr("Maximize"));
    m_closeButton = new TitleBarButton(TitleBarButton::Role::Close, this);
    m_closeButton->setObjectName("TitleBarCloseButton");
    m_closeButton->setToolTip(tr("Close"));

    connect(m_minimizeButton, &QToolButton::clicked, this, &TitleBar::minimizeRequested);
    connect(m_maximizeButton, &QToolButton::clicked, this, &TitleBar::maximizeRequested);
    connect(m_closeButton, &QToolButton::clicked, this, &TitleBar::closeRequested);

    auto* layout = new QHBoxLayout(this);
    layout->setContentsMargins(4, 0, 0, 0);
    layout->setSpacing(0);
    layout->addStretch(1);
    layout->addWidget(m_minimizeButton);
    layout->addWidget(m_maximizeButton);
    layout->addWidget(m_closeButton);
}

void TitleBar::setMenuBar(QMenuBar* menuBar)
{
    if (m_menuBar == menuBar)
        return;

    if (m_menuBar)
    {
        layout()->removeWidget(m_menuBar);
        m_menuBar->hide();
    }

    m_menuBar = menuBar;
    if (!m_menuBar)
        return;

    m_menuBar->setParent(this);
    m_menuBar->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);
    qobject_cast<QHBoxLayout*>(layout())->insertWidget(0, m_menuBar);
}

void TitleBar::setMaximized(bool maximized)
{
    m_maximizeButton->setMaximized(maximized);
    m_maximizeButton->setToolTip(maximized ? tr("Restore") : tr("Maximize"));
}

void TitleBar::mouseDoubleClickEvent(QMouseEvent* event)
{
    if (event->button() == Qt::LeftButton)
    {
        emit maximizeRequested();
        event->accept();
        return;
    }
    QWidget::mouseDoubleClickEvent(event);
}

void TitleBar::paintEvent(QPaintEvent* event)
{
    Q_UNUSED(event)

    const ThemeColors colors = getThemeColors(SettingsManager::instance().theme());

    QPainter painter(this);
    painter.fillRect(rect(), colors.toolbarBg);
    painter.setPen(colors.separator);
    painter.drawLine(0, height() - 1, width(), height() - 1);
}
