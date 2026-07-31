#ifndef DROPZONEOVERLAY_H
#define DROPZONEOVERLAY_H

#include "splitview.h"

#include <QColor>
#include <QRect>
#include <QWidget>

class DropZoneOverlay : public QWidget
{
    Q_OBJECT

public:
    explicit DropZoneOverlay(QWidget* parent, QWidget* viewWidget);

    void setTargetPane(QTabWidget* pane, SplitView::DropZone zone, const QColor& color);
    void clearZone();
    void updateOverlayPosition();

protected:
    void paintEvent(QPaintEvent* event) override;

private:
    SplitView::DropZone m_zone = SplitView::DropZone::None;
    QRect m_paneRect;
    QColor m_color;
    QWidget* m_viewWidget = nullptr;
};

#endif // DROPZONEOVERLAY_H
