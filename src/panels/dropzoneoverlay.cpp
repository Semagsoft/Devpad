#include "dropzoneoverlay.h"

#include <QPainter>
#include <QPainterPath>
#include <QTabWidget>
#include <QWidget>

DropZoneOverlay::DropZoneOverlay(QWidget* parent, QWidget* viewWidget) : QWidget(parent), m_viewWidget(viewWidget)
{
    setAttribute(Qt::WA_TransparentForMouseEvents);
    setAttribute(Qt::WA_TranslucentBackground);
    raise();
}

void DropZoneOverlay::setTargetPane(QTabWidget* pane, SplitView::DropZone zone, const QColor& color)
{
    m_zone = zone;
    m_color = color;
    if (pane && zone != SplitView::DropZone::None)
    {
        QPoint topLeft = pane->mapTo(parentWidget(), QPoint(0, 0));
        m_paneRect = QRect(topLeft, pane->size());
    }
    else
    {
        m_paneRect = QRect();
    }
    repaint();
}

void DropZoneOverlay::clearZone()
{
    m_zone = SplitView::DropZone::None;
    m_paneRect = QRect();
    repaint();
}

void DropZoneOverlay::updateOverlayPosition()
{
    if (m_viewWidget)
    {
        QPoint tl = m_viewWidget->mapTo(parentWidget(), QPoint(0, 0));
        setGeometry(tl.x(), tl.y(), m_viewWidget->width(), m_viewWidget->height());
    }
}

void DropZoneOverlay::paintEvent(QPaintEvent* event)
{
    Q_UNUSED(event)
    if (m_zone == SplitView::DropZone::None || !m_paneRect.isValid())
        return;

    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);

    QRect r = m_paneRect;
    QColor accent = m_color;

    if (m_zone == SplitView::DropZone::Center)
    {
        QColor fill = accent;
        fill.setAlpha(80);
        p.fillRect(r, fill);

        int m = std::min(r.width(), r.height()) / 5;
        QRect inner = r.adjusted(m, m, -m, -m);
        QColor border = accent;
        border.setAlpha(220);
        QPen pen(border, 4);
        p.setPen(pen);
        p.setBrush(Qt::NoBrush);
        p.drawRoundedRect(inner, 10, 10);
        return;
    }

    QRect zoneRect;
    switch (m_zone)
    {
    case SplitView::DropZone::Left:
        zoneRect = QRect(r.left(), r.top(), r.width() / 3, r.height());
        break;
    case SplitView::DropZone::Right:
        zoneRect = QRect(r.right() - r.width() / 3, r.top(), r.width() / 3, r.height());
        break;
    case SplitView::DropZone::Top:
        zoneRect = QRect(r.left(), r.top(), r.width(), r.height() / 3);
        break;
    case SplitView::DropZone::Bottom:
        zoneRect = QRect(r.left(), r.bottom() - r.height() / 3, r.width(), r.height() / 3);
        break;
    default:
        return;
    }

    QColor fill = accent;
    fill.setAlpha(140);
    p.fillRect(zoneRect, fill);

    QColor line = accent;
    line.setAlpha(230);
    QPen linePen(line, 5);
    p.setPen(linePen);

    switch (m_zone)
    {
    case SplitView::DropZone::Left:
        p.drawLine(zoneRect.topRight(), zoneRect.bottomRight());
        break;
    case SplitView::DropZone::Right:
        p.drawLine(zoneRect.topLeft(), zoneRect.bottomLeft());
        break;
    case SplitView::DropZone::Top:
        p.drawLine(zoneRect.bottomLeft(), zoneRect.bottomRight());
        break;
    case SplitView::DropZone::Bottom:
        p.drawLine(zoneRect.topLeft(), zoneRect.topRight());
        break;
    default:
        break;
    }
}
