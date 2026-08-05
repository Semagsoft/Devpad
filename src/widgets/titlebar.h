#ifndef TITLEBAR_H
#define TITLEBAR_H

#include <QToolButton>
#include <QWidget>

class QLabel;
class QMenuBar;

// Window control button that paints its glyph with QPainter so it adapts to
// every theme. QSS only handles the background/hover states.
class TitleBarButton : public QToolButton
{
    Q_OBJECT

public:
    enum class Role
    {
        Minimize,
        Maximize,
        Restore,
        Close
    };

    explicit TitleBarButton(Role role, QWidget* parent = nullptr);

    void setMaximized(bool maximized);

protected:
    void paintEvent(QPaintEvent* event) override;

private:
    Role m_role = Role::Minimize;
};

class TitleBar : public QWidget
{
    Q_OBJECT

public:
    explicit TitleBar(QWidget* parent = nullptr);

    static constexpr int Height = 32;

    void setMenuBar(QMenuBar* menuBar);
    QMenuBar* menuBarWidget() const
    {
        return m_menuBar;
    }
    QToolButton* minimizeButton() const
    {
        return m_minimizeButton;
    }
    QToolButton* maximizeButton() const
    {
        return m_maximizeButton;
    }
    QToolButton* closeButton() const
    {
        return m_closeButton;
    }

    void setMaximized(bool maximized);
    void setTitleText(const QString& text);

signals:
    void minimizeRequested();
    void maximizeRequested();
    void closeRequested();

protected:
    void mouseDoubleClickEvent(QMouseEvent* event) override;
    void paintEvent(QPaintEvent* event) override;

private:
    QMenuBar* m_menuBar = nullptr;
    QLabel* m_titleLabel = nullptr;
    TitleBarButton* m_minimizeButton = nullptr;
    TitleBarButton* m_maximizeButton = nullptr;
    TitleBarButton* m_closeButton = nullptr;
};

#endif // TITLEBAR_H
