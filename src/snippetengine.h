#ifndef SNIPPETENGINE_H
#define SNIPPETENGINE_H

#include "snippet.h"

#include <QList>
#include <QObject>

class CodeEditor;

class SnippetEngine : public QObject
{
    Q_OBJECT

public:
    explicit SnippetEngine(CodeEditor* editor);

    void insertSnippet(const Snippet& snippet);
    bool isActive() const { return m_snippetActive; }
    void exitSnippetMode();

    bool handleKeyPress(int key, Qt::KeyboardModifiers modifiers);

    static constexpr int SNIPPET_INDICATOR = 21;

signals:
    void snippetModeChanged(bool active);

private:
    struct TabStopInfo
    {
        int number = 0;
        int pos = 0;
        int length = 0;
        QString defaultValue;
    };

    void enterSnippetMode(const Snippet::ExpandedSnippet& expanded, int insertPos, const QString& triggerText = QString());
    void advanceTabStop();
    void retreatTabStop();
    void clearSnippetMarkers();
    void selectTabStopRange(int pos, int len);
    void recalculateTabStopPositions();

    CodeEditor* m_editor;

    bool m_snippetActive = false;
    int m_currentTabStopIdx = -1;
    int m_snippetStartPos = 0;
    int m_snippetEndPos = 0;
    QList<TabStopInfo> m_tabStopInfos;
};

#endif
