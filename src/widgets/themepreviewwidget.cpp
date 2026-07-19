#include "themepreviewwidget.h"

#include <QPainter>

ThemePreviewWidget::ThemePreviewWidget(QWidget* parent) : QWidget(parent)
{
    setMinimumHeight(180);
    setMaximumHeight(220);
}

void ThemePreviewWidget::setThemeColors(const ThemeColors& colors)
{
    m_colors = colors;
    update();
}

static QColor blendPreview(const QColor& a, const QColor& b, double t)
{
    auto clamp = [](int v) { return v < 0 ? 0 : v > 255 ? 255 : v; };
    return QColor(clamp(qRound(a.red() * (1.0 - t) + b.red() * t)), clamp(qRound(a.green() * (1.0 - t) + b.green() * t)),
                  clamp(qRound(a.blue() * (1.0 - t) + b.blue() * t)));
}

void ThemePreviewWidget::paintEvent(QPaintEvent*)
{
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing, true);

    int w = width();
    int h = height();
    const auto& c = m_colors;

    // ── Toolbar area ──
    int toolbarH = 22;
    QRect toolbarRect(0, 0, w, toolbarH);
    p.fillRect(toolbarRect, c.toolbarBg);

    // Toolbar buttons
    QColor btnColor = blendPreview(c.toolbarBg, c.selectionBg, 0.25);
    int btnY = 4;
    for (int i = 0; i < 5; ++i)
    {
        QRect btn(i * 22 + 6, btnY, 14, 14);
        p.fillRect(btn, btnColor);
    }

    // ── Tab bar area ──
    int tabH = 20;
    int tabY = toolbarH;
    QRect tabsBg(0, tabY, w, tabH);
    p.fillRect(tabsBg, c.tabBg);

    // Inactive tab
    int tabW = 70;
    int tabGap = 2;
    QRect tab1(tabGap, tabY + 2, tabW, tabH - 2);
    p.fillRect(tab1, c.tabBg);
    p.setPen(c.tabFg);
    p.drawText(tab1.adjusted(6, 0, 0, 0), Qt::AlignVCenter, "file1.cpp");

    // Active tab
    QRect tab2(tabGap + tabW + tabGap, tabY, tabW + 10, tabH);
    p.fillRect(tab2, c.tabBgActive);
    p.setPen(c.tabFgActive);
    p.drawText(tab2.adjusted(6, 0, 0, 0), Qt::AlignVCenter, "main.rs");

    // Tab separator line
    p.setPen(c.tabBorder);
    p.drawLine(0, tabY + tabH, w, tabY + tabH);

    // ── Editor area ──
    int editorY = tabY + tabH + 1;
    int editorH = h - editorY - 22;
    QRect editorBg(0, editorY, w, editorH);
    p.fillRect(editorBg, c.background);

    // Line highlight on line 3
    int lineH = 16;
    int selLineY = editorY + 2 * lineH;
    p.fillRect(QRect(0, selLineY, w, lineH), c.lineHighlight);

    // Gutter/margin
    int marginW = 28;
    p.fillRect(QRect(0, editorY, marginW, editorH), c.marginBg);

    // Line numbers
    QFont mono("Monospace", 7);
    mono.setStyleHint(QFont::Monospace);
    p.setFont(mono);
    p.setPen(c.marginFg);
    for (int i = 0; i < 7; ++i)
    {
        int ly = editorY + i * lineH + 12;
        QRect numRect(2, ly - 10, marginW - 6, 12);
        p.drawText(numRect, Qt::AlignRight | Qt::AlignVCenter, QString::number(i + 1));
    }

    // Code content
    int cx = marginW + 6;

    struct Token
    {
        QString text;
        QColor color;
    };
    struct Line
    {
        QVector<Token> tokens;
    };

    Line lines[7];
    lines[0] = {{{"use ", c.keyword}, {"std::io;\n", c.function}}};
    lines[1] = {{{"\n", c.foreground}}};
    lines[2] = {{{"fn ", c.keyword}, {"main", c.function}, {"() ", c.operator_}, {"{\n", c.operator_}}};
    lines[3] = {{{"    let ", c.keyword},
                 {"mut ", c.keyword},
                 {"input", c.foreground},
                 {" = ", c.operator_},
                 {"String", c.function},
                 {"::new();\n", c.function}}};
    lines[4] = {{{"    println!(\"", c.function}, {"hello ", c.string}, {"{}\", ", c.function}, {"input", c.foreground}, {");\n", c.function}}};
    lines[5] = {{{"    let ", c.keyword}, {"x", c.foreground}, {" = ", c.operator_}, {"42", c.number}, {";\n", c.operator_}}};
    lines[6] = {{{"}\n", c.operator_}}};

    // Selection highlight on the "hello" string in line 5
    int selYOffset = 4 * lineH;
    int selY = editorY + selYOffset + 2;
    int selX = cx + p.fontMetrics().horizontalAdvance("    println!(\"");
    QRect selRect(selX, selY, p.fontMetrics().horizontalAdvance("hello "), lineH - 4);
    p.fillRect(selRect, c.selectionBg);

    // Caret on line 6 at end
    int caretLineY = editorY + 5 * lineH;
    int caretX = cx + p.fontMetrics().horizontalAdvance("    let x = ");
    p.fillRect(caretX, caretLineY + 2, 2, lineH - 4, c.caret);

    int ly = editorY + 12;
    for (int li = 0; li < 7; ++li)
    {
        int tx = cx;
        for (const auto& token : lines[li].tokens)
        {
            bool isSelected = (li == 4 && token.text == "hello ");
            p.setPen(isSelected ? c.selectionFg : token.color);
            p.drawText(tx, ly, token.text);
            tx += p.fontMetrics().horizontalAdvance(token.text);
        }
        ly += lineH;
    }

    // ── Scrollbar ──
    int sbW = 8;
    int sbX = w - sbW;
    p.fillRect(QRect(sbX, editorY, sbW, editorH), c.scrollbarBg);
    int sbHandleH = editorH / 3;
    int sbHandleY = editorY + editorH / 3;
    p.fillRect(QRect(sbX + 1, sbHandleY, sbW - 2, sbHandleH), c.scrollbarHandle);

    // ── Status bar ──
    int statusY = h - 22;
    p.fillRect(QRect(0, statusY, w, 22), c.statusbarBg);

    p.setPen(c.statusbarFg);
    QFont statusFont("Sans", 7);
    p.setFont(statusFont);
    p.drawText(8, statusY + 14, "Line: 7, Col: 1");
    p.drawText(w - 100, statusY + 14, "UTF-8");

    // Status bar separator
    p.setPen(c.separator);
    p.drawLine(0, statusY, w, statusY);

    // Border around widget
    p.setPen(c.border);
    p.drawRect(0, 0, w - 1, h - 1);
}
