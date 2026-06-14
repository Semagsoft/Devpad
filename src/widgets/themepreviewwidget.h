#ifndef THEMEPREVIEWWIDGET_H
#define THEMEPREVIEWWIDGET_H

#include <QWidget>
#include "theme.h"

class ThemePreviewWidget : public QWidget {
    Q_OBJECT
public:
    explicit ThemePreviewWidget(QWidget *parent = nullptr);
    void setThemeColors(const ThemeColors &colors);

protected:
    void paintEvent(QPaintEvent *event) override;

private:
    ThemeColors m_colors;
};

#endif
