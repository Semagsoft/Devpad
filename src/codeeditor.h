#ifndef CODEEDITOR_H
#define CODEEDITOR_H

#include "settingsmanager.h"
#include "snippet.h"
#include "theme.h"

#include <QScopedPointer>

#include <Qsci/qsciapis.h>
#include <Qsci/qsciglobal.h>
#include <Qsci/qscilexer.h>
#include <Qsci/qsciscintilla.h>

class CodeEditor : public QsciScintilla
{
    Q_OBJECT

public:
    explicit CodeEditor(QWidget* parent = nullptr);
    ~CodeEditor() override;

    static const int BOOKMARK_MARGIN = 1;
    static const int MARKER_BOOKMARK = 0;

    const QString& fileName() const
    {
        return m_fileName;
    }
    void setFileName(const QString& name)
    {
        m_fileName = name;
    }

    void setSyntax(const QString& language);
    const QString& syntax() const
    {
        return m_syntax;
    }
    void setEncoding(const QString& encoding)
    {
        m_encoding = encoding;
    }
    const QString& encoding() const
    {
        return m_encoding;
    }

    void setLineNumbersVisible(bool visible);
    void applyTheme(ThemeId themeId);
    void applyFont(const QFont& font);
    void setScrollPastEnd(bool enabled);
    void setCodeFolding(bool enabled);
    void setWhitespaceVisible(bool visible);
    void setupAutoCompletion(bool enabled, int threshold, bool caseSensitive);
    void setCursorStyle(CursorStyle style);
    void setCursorBlinking(bool enabled);
    void setHighlightCurrentLine(bool enabled);
    void setVerticalEdge(bool enabled, int column);
    void setAutoCloseBrackets(bool enabled);
    void setReadOnlyMode(bool enabled);
    bool isReadOnlyMode() const;

    void toggleBookmark();
    void toggleBookmark(int line);
    bool hasBookmark(int line) const;
    void nextBookmark();
    void prevBookmark();
    void clearBookmarks();
    QList<int> bookmarkLines() const;
    void setBookmarks(const QList<int>& lines);

    // Snippet support
    void insertSnippet(const Snippet& snippet);
    bool isSnippetMode() const { return m_snippetActive; }
    void exitSnippetMode();
    void registerSnippetAutoCompletion(const QList<Snippet>& snippets);

signals:
    void fileDropped(const QString& filePath);
    void bookmarksChanged();
    void snippetModeChanged(bool active);
    void findRequested();
    void replaceRequested();
    void goToLineRequested();
    void insertSnippetRequested();

public slots:
    void forceModified();
    void zoomIn() override;
    void zoomOut() override;
    void zoomReset();

protected:
    void dragEnterEvent(QDragEnterEvent* event) override;
    void dropEvent(QDropEvent* event) override;
    void keyPressEvent(QKeyEvent* event) override;
    void contextMenuEvent(QContextMenuEvent* event) override;

private:
    QString m_fileName;
    QString m_syntax;
    QString m_encoding;
    QFont m_defaultFont;
    ThemeId m_themeId;
    QScopedPointer<QsciLexer> m_lexer;
    QScopedPointer<QsciAPIs> m_apis;
    bool m_lineNumbersVisible = true;
    bool m_autoCloseBrackets = true;

    struct BracketContext
    {
        bool inString = false;
        bool inCharLiteral = false;
        bool inComment = false;
        bool inBlockComment = false;
    };

    BracketContext contextAtPosition(int pos) const;
    bool handleAutoClose(QChar ch, int pos);
    bool handleBracketSkip(QChar ch, int pos);

    void setupEditor();
    void applyLexerTheme();
    void updateLineNumberWidth();

    // Snippet infrastructure
    static constexpr int SNIPPET_INDICATOR = 21;

    struct TabStopInfo
    {
        int number = 0;
        int pos = 0;
        int length = 0;
        QString defaultValue;
    };

    bool m_snippetActive = false;
    int m_currentTabStopIdx = -1;
    int m_snippetStartPos = 0;
    int m_snippetEndPos = 0;
    QList<TabStopInfo> m_tabStopInfos;

    void enterSnippetMode(const Snippet::ExpandedSnippet& expanded, int insertPos, const QString& triggerText = QString());
    void advanceTabStop();
    void retreatTabStop();
    void clearSnippetMarkers();
    void selectTabStopRange(int pos, int len);
    void recalculateTabStopPositions();
};

#endif // CODEEDITOR_H
