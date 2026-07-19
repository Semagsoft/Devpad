#ifndef SEARCHMANAGER_H
#define SEARCHMANAGER_H

#include <QObject>
#include <QPointer>
#include <QString>

class QMainWindow;

class CodeEditor;
class QsciScintilla;
class TabManager;

class SearchManager : public QObject
{
    Q_OBJECT

public:
    explicit SearchManager(QMainWindow* parent, TabManager* tabManager);
    ~SearchManager();

    void showFindDialog();
    void showReplaceDialog();
    void findNext();
    void findPrevious();
    bool hasActiveSearch() const
    {
        return !m_lastSearchText.isEmpty();
    }

    const QString& lastSearchText() const
    {
        return m_lastSearchText;
    }
    bool lastUseRegex() const
    {
        return m_lastUseRegex;
    }
    bool lastMatchCase() const
    {
        return m_lastMatchCase;
    }
    bool lastMatchWholeWord() const
    {
        return m_lastMatchWholeWord;
    }

    void setLastSearch(const QString& text, bool useRegex = false, bool matchCase = false, bool matchWholeWord = false)
    {
        m_lastSearchText = text;
        m_lastUseRegex = useRegex;
        m_lastMatchCase = matchCase;
        m_lastMatchWholeWord = matchWholeWord;
    }

private:
    void performSearch(const QString& text, bool regex, bool caseSensitive, bool wholeWord, bool searchForward);
    CodeEditor* currentEditor() const;

    QMainWindow* m_parent;
    TabManager* m_tabManager;

    QString m_lastSearchText;
    bool m_lastUseRegex = false;
    bool m_lastMatchCase = false;
    bool m_lastMatchWholeWord = false;
};

#endif // SEARCHMANAGER_H
