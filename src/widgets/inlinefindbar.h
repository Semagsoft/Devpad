#ifndef INLINEFINDBAR_H
#define INLINEFINDBAR_H

#include <QList>
#include <QPair>
#include <QString>
#include <QWidget>

class QLineEdit;
class QToolButton;
class QPushButton;
class QLabel;
class QTimer;
class CodeEditor;

class InlineFindBar : public QWidget
{
    Q_OBJECT

public:
    explicit InlineFindBar(CodeEditor* editor, QWidget* parent = nullptr);
    ~InlineFindBar() override;

    void showFindMode(const QString& selectedText = QString());
    void showReplaceMode(const QString& selectedText = QString());
    void closeBar();
    bool isReplaceMode() const
    {
        return m_replaceMode;
    }

    QString searchText() const;
    bool matchCase() const;
    bool matchWholeWord() const;
    bool useRegex() const;

    void findNext();
    void findPrevious();

signals:
    void closed();

protected:
    void resizeEvent(QResizeEvent* event) override;
    bool eventFilter(QObject* obj, QEvent* event) override;

private slots:
    void onFindTextChanged(const QString& text);
    void onDebounceTimeout();
    void onFindNextClicked();
    void onFindPreviousClicked();
    void onReplaceClicked();
    void onReplaceAllClicked();
    void onCloseClicked();

private:
    void setupUI();
    void setReplaceMode(bool replace);
    void performSearch();
    void clearHighlights();
    void navigateToMatch(int index);
    void saveState();
    void restoreState();
    void updateIcons();
    QColor highlightColor() const;

    static QIcon paintIcon(const QColor& fg, const QString& text, bool underline = false, bool brackets = false);

    CodeEditor* m_editor;

    // Find widgets
    QLineEdit* m_findInput;
    QToolButton* m_prevBtn;
    QToolButton* m_nextBtn;
    QToolButton* m_caseBtn;
    QToolButton* m_wordBtn;
    QToolButton* m_regexBtn;
    QLabel* m_matchLabel;

    // Replace widgets — grouped in a container
    QWidget* m_replaceSection;
    QLineEdit* m_replaceInput;
    QPushButton* m_replaceBtn;
    QPushButton* m_replaceAllBtn;

    QToolButton* m_closeBtn;

    QTimer* m_debounceTimer;
    bool m_replaceMode = false;

    static constexpr int FIND_INDICATOR = 22;
    static constexpr int DEBOUNCE_MS = 100;

    struct MatchRange
    {
        int pos;
        int len;
    };
    QList<MatchRange> m_matches;
    int m_currentMatch = -1;
};

#endif // INLINEFINDBAR_H
