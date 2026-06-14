#include "themepreviewwidget.h"
#include <QPainter>

ThemePreviewWidget::ThemePreviewWidget(QWidget *parent)
    : QWidget(parent)
{
    setMinimumHeight(110);
    setMaximumHeight(130);
}

void ThemePreviewWidget::setThemeColors(const ThemeColors &colors)
{
    m_colors = colors;
    update();
}

void ThemePreviewWidget::paintEvent(QPaintEvent *)
{
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing, false);

    int w = width();
    int h = height();

    // Background
    p.fillRect(rect(), m_colors.background);

    // Margin/gutter area
    int marginWidth = 30;
    QRect gutter(0, 0, marginWidth, h);
    p.fillRect(gutter, m_colors.marginBg);

    // Line numbers
    QFont mono("Monospace", 8);
    mono.setStyleHint(QFont::Monospace);
    p.setFont(mono);
    p.setPen(m_colors.marginFg);
    int lineY = 14;
    for (int i = 1; i <= 5; ++i) {
        QRect numRect(0, lineY - 10, marginWidth - 4, 14);
        p.drawText(numRect, Qt::AlignRight | Qt::AlignVCenter, QString::number(i));
        lineY += 18;
    }

    // Content area
    int cx = marginWidth + 8;

    // "Code" lines
    struct Token { QString text; QColor color; };
    struct Line { QVector<Token> tokens; };

    Line lines[5];
    lines[0] = {{{"fn ", m_colors.keyword}, {"main() ", m_colors.function}, {"{\n", m_colors.operator_}}};
    lines[1] = {{{"    let ", m_colors.keyword}, {"x", m_colors.foreground}, {" = ", m_colors.operator_}, {"42", m_colors.number}, {";", m_colors.operator_}}};
    lines[2] = {{{"    println!(\"", m_colors.function}, {"hello", m_colors.string}, {"\");", m_colors.function}}};
    lines[3] = {{{"    let ", m_colors.keyword}, {"y", m_colors.foreground}, {" = ", m_colors.operator_}, {"x", m_colors.foreground}, {" + ", m_colors.operator_}, {"1", m_colors.number}, {";", m_colors.operator_}}};
    lines[4] = {{{"}", m_colors.operator_}}};

    // Selection highlight on line 2
    int selY = 14 + 1 * 18;
    QRect selRect(cx + 50, selY - 12, 80, 16);
    p.fillRect(selRect, m_colors.selectionBg);

    lineY = 14;
    for (int li = 0; li < 5; ++li) {
        int tx = cx;
        for (const auto &token : lines[li].tokens) {
            if (li == 1 && token.text == "hello") {
                p.setPen(m_colors.selectionFg);
            } else {
                p.setPen(token.color);
            }
            p.drawText(tx, lineY, token.text);
            tx += p.fontMetrics().horizontalAdvance(token.text);
        }
        lineY += 18;
    }

    // Caret on line 4
    int caretX = cx + p.fontMetrics().horizontalAdvance("    let y = x + ");
    p.fillRect(caretX, 14 + 3 * 18 - 12, 2, 14, m_colors.caret);

    // Bottom border
    p.setPen(m_colors.separator);
    p.drawLine(0, h - 1, w, h - 1);
}
