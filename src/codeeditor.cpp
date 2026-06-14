#include "codeeditor.h"

#include "languageinfo.h"
#include "settingsmanager.h"
#include "snippetmanager.h"
#include "theme.h"

#include <QApplication>
#include <QDragEnterEvent>
#include <QDropEvent>
#include <QFileInfo>
#include <QFont>
#include <QKeyEvent>
#include <QMimeData>

#include <Qsci/qsciapis.h>

namespace
{
constexpr int SCI_SETSCROLLPASTEND = 2694;
constexpr int SCI_SETCARETSTYLE = 2068;
constexpr int SCI_SETCARETPERIOD = 2075;
constexpr int SCI_SETEDGEMODE = 2360;
constexpr int SCI_SETEDGECOLUMN = 2361;
constexpr int SCI_SETEDGECOLOUR = 2362;
constexpr int SCI_INDICSETSTYLE = 2088;
constexpr int SCI_INDICSETFORE = 2089;
constexpr int SCI_INDICATORFILLRANGE = 2100;
constexpr int SCI_INDICATORCLEARRANGE = 2101;
constexpr int SCI_INDICATORSTART = 2102;
constexpr int SCI_INDICATOREND = 2103;
constexpr int SCI_SETSELECTIONSTART = 2142;
constexpr int SCI_SETSELECTIONEND = 2143;
constexpr int SCI_GETSELECTIONSTART = 2144;
constexpr int SCI_GETSELECTIONEND = 2145;
constexpr int SCI_GETCURRENTPOS = 2008;
constexpr int SCI_SETCURRENTPOS = 2141;
constexpr int SCI_GETANCHOR = 2009;
constexpr int SCI_SETANCHOR = 2025;
constexpr int SCI_AUTOCSETAUTOMATIC = 2231;

constexpr int INDIC_HIDDEN = 8;

constexpr int CARETSTYLE_LINE = 1;
constexpr int CARETSTYLE_BLOCK = 2;
constexpr int CARETSTYLE_UNDERLINE = 4;

constexpr int EDGE_NONE = 0;

const QString SNIPPET_MARKER = QStringLiteral("\u00ABsnip\u00BB");
} // namespace

CodeEditor::CodeEditor(QWidget* parent) : QsciScintilla(parent), m_encoding("UTF-8"), m_themeId(ThemeId::Light), m_lineNumbersVisible(true)
{
    setupEditor();
    setAcceptDrops(true);
    connect(this, &QsciScintilla::linesChanged, this, &CodeEditor::updateLineNumberWidth);
    connect(this, &QsciScintilla::marginClicked, this,
            [this](int margin, int line, Qt::KeyboardModifiers)
            {
                if (margin == BOOKMARK_MARGIN)
                {
                    toggleBookmark(line);
                }
            });

    // Set up invisible indicator for snippet tab stops
    SendScintilla(SCI_INDICSETSTYLE, static_cast<unsigned long>(SNIPPET_INDICATOR), static_cast<unsigned long>(INDIC_HIDDEN));
    SendScintilla(SCI_INDICSETFORE, static_cast<unsigned long>(SNIPPET_INDICATOR), static_cast<unsigned long>(0));

    // Connect autocompletion selection signal to handle snippet expansion
    connect(this, static_cast<void(QsciScintillaBase::*)(const char*, int)>(&QsciScintillaBase::SCN_AUTOCSELECTION), this, [this](const char* selection, int position) {
        if (!SettingsManager::instance().predictiveSnippets() || !selection)
            return;

        QString selectedText = QString::fromUtf8(selection);
        if (selectedText.endsWith(SNIPPET_MARKER))
        {
            QString prefix = selectedText.left(selectedText.length() - SNIPPET_MARKER.length());
            if (SnippetManager* sm = SnippetManager::instance())
            {
                QList<Snippet> candidates = sm->snippetsByPrefix(prefix, m_syntax);
                for (const Snippet& snip : candidates)
                {
                    if (snip.prefix == prefix)
                    {
                        int triggerLen = prefix.length();
                        int triggerStart = position - triggerLen;
                        if (triggerStart >= 0)
                        {
                            Snippet::ExpandedSnippet expanded = snip.expand();
                            enterSnippetMode(expanded, position, prefix);
                        }
                        return;
                    }
                }
            }
        }
    });
}

CodeEditor::~CodeEditor() = default;

void CodeEditor::dragEnterEvent(QDragEnterEvent* event)
{
    if (event->mimeData()->hasUrls())
    {
        event->acceptProposedAction();
    }
    else if (event->mimeData()->hasText())
    {
        event->acceptProposedAction();
    }
}

void CodeEditor::dropEvent(QDropEvent* event)
{
    const QMimeData* mimeData = event->mimeData();
    if (mimeData->hasUrls())
    {
        for (const QUrl& url : mimeData->urls())
        {
            QString fileName = url.toLocalFile();
            if (!fileName.isEmpty() && QFileInfo(fileName).isFile())
            {
                emit fileDropped(fileName);
                event->acceptProposedAction();
                return;
            }
        }
    }
    if (mimeData->hasText())
    {
        insert(mimeData->text());
        event->acceptProposedAction();
    }
}

void CodeEditor::setupEditor()
{
    m_defaultFont = QFont("Monospace", 12);
    setFont(m_defaultFont);
    setMarginsFont(m_defaultFont);

    setMarginLineNumbers(0, true);
    setLineNumbersVisible(true);
    setMarginsBackgroundColor(QColor(240, 240, 240));
    setMarginsForegroundColor(Qt::darkGray);

    setFolding(QsciScintilla::BoxedTreeFoldStyle);
    setFoldMarginColors(QColor(220, 220, 220), QColor(200, 200, 200));

    setBraceMatching(QsciScintilla::SloppyBraceMatch);
    setMatchedBraceBackgroundColor(QColor(255, 255, 200));
    setMatchedBraceForegroundColor(Qt::darkGreen);

    setAutoIndent(true);
    setTabWidth(4);
    setIndentationsUseTabs(false);

    setCaretLineVisible(true);
    setCaretLineBackgroundColor(QColor(232, 242, 254));

    setMarginType(BOOKMARK_MARGIN, QsciScintilla::SymbolMargin);
    setMarginWidth(BOOKMARK_MARGIN, 16);
    setMarginSensitivity(BOOKMARK_MARGIN, true);
    setMarginMarkerMask(BOOKMARK_MARGIN, (1 << MARKER_BOOKMARK));
    markerDefine(QsciScintilla::SC_MARK_BOOKMARK, MARKER_BOOKMARK);
    setMarkerForegroundColor(QColor(255, 160, 0), MARKER_BOOKMARK);
    setMarkerBackgroundColor(QColor(255, 200, 100), MARKER_BOOKMARK);

    // Enable automatic autocompletion list mode
    SendScintilla(SCI_AUTOCSETAUTOMATIC, 1);
}

void CodeEditor::applyTheme(ThemeId themeId)
{
    m_themeId = themeId;
    ThemeColors colors = getThemeColors(themeId);

    setMarginsBackgroundColor(colors.marginBg);
    setMarginsForegroundColor(colors.marginFg);
    setFoldMarginColors(colors.foldMarginBg, colors.foldMarginBgAlt);
    setMatchedBraceBackgroundColor(colors.matchedBraceBg);
    setMatchedBraceForegroundColor(colors.matchedBraceFg);
    setCaretLineBackgroundColor(colors.lineHighlight);
    setCaretForegroundColor(colors.caret);
    setPaper(colors.background);
    setColor(colors.foreground);

    if (SettingsManager::instance().verticalEdgeEnabled())
    {
        SendScintilla(SCI_SETEDGEMODE, EDGE_LINE);
        SendScintilla(SCI_SETEDGECOLOUR, colors.separator.red() | (colors.separator.green() << 8) | (colors.separator.blue() << 16));
    }

    applyLexerTheme();
}

void CodeEditor::applyLexerTheme()
{
    QsciLexer* lexer = this->lexer();
    if (!lexer)
        return;

    ThemeColors colors = getThemeColors(m_themeId);

    lexer->setDefaultColor(colors.foreground);
    lexer->setDefaultPaper(colors.background);
    lexer->setPaper(colors.background);

    const char* lexerName = lexer->metaObject()->className();
    const auto& cache = themeApplicatorCache();
    auto it = cache.find(QString::fromLatin1(lexerName));
    if (it != cache.end())
    {
        it.value()(lexer, colors);
    }
}

void CodeEditor::setSyntax(const QString& language)
{
    m_syntax = language;

    m_apis.reset();
    m_lexer.reset();

    const LanguageInfo* lang = findLanguage(language);
    if (!lang)
    {
        setLexer(nullptr);
        return;
    }

    QsciLexer* lexer = lang->factory(this);
    lexer->setFont(m_defaultFont);
    m_lexer.reset(lexer);

    const QStringList& keywords = lang->keywords();
    if (!keywords.isEmpty())
    {
        m_apis.reset(new QsciAPIs(lexer));
        for (const auto& kw : keywords)
        {
            m_apis->add(kw);
        }
        m_apis->prepare();
        lexer->setAPIs(m_apis.data());
    }

    // Register snippet auto-completion entries
    if (SnippetManager* sm = SnippetManager::instance())
    {
        QList<Snippet> snippets = sm->snippetsForLanguage(language);
        if (!snippets.isEmpty())
        {
            registerSnippetAutoCompletion(snippets);
        }
    }

    setLexer(lexer);
    applyLexerTheme();
}

void CodeEditor::registerSnippetAutoCompletion(const QList<Snippet>& snippets)
{
    if (snippets.isEmpty() || !SettingsManager::instance().predictiveSnippets())
        return;

    if (!m_apis && m_lexer)
    {
        m_apis.reset(new QsciAPIs(m_lexer.data()));
    }

    if (!m_apis)
        return;

    for (const Snippet& s : snippets)
    {
        m_apis->add(s.prefix + SNIPPET_MARKER);
    }
    m_apis->prepare();
    if (m_lexer)
    {
        m_lexer->setAPIs(m_apis.data());
    }
}

void CodeEditor::setLineNumbersVisible(bool visible)
{
    m_lineNumbersVisible = visible;
    if (visible)
    {
        setMarginType(0, QsciScintilla::NumberMargin);
        setMarginLineNumbers(0, true);
        updateLineNumberWidth();
    }
    else
    {
        setMarginType(0, QsciScintilla::TextMargin);
        setMarginWidth(0, 0);
    }
}

void CodeEditor::updateLineNumberWidth()
{
    if (!m_lineNumbersVisible)
        return;
    int lineCount = qMax(lines(), 1);
    int digits = QString::number(lineCount).length();
    QFontMetrics fm(m_defaultFont);
    int width = fm.horizontalAdvance(QString(digits, '9')) + 10;
    setMarginWidth(0, width);
}

void CodeEditor::forceModified()
{
    int line, index;
    getCursorPosition(&line, &index);

    bool blocked = blockSignals(true);
    beginUndoAction();
    int endPos = SendScintilla(SCI_GETLENGTH);
    SendScintilla(SCI_INSERTTEXT, endPos, QByteArray(" ").constData());
    SendScintilla(SCI_SETCURRENTPOS, endPos + 1);
    SendScintilla(SCI_SETANCHOR, endPos + 1);
    SendScintilla(SCI_DELETEBACK);
    endUndoAction();
    blockSignals(blocked);

    setCursorPosition(line, index);
}

void CodeEditor::zoomIn()
{
    QsciScintilla::zoomIn();
}

void CodeEditor::zoomOut()
{
    QsciScintilla::zoomOut();
}

void CodeEditor::zoomReset()
{
    zoomTo(0);
    QFont font = SettingsManager::instance().defaultFont();
    m_defaultFont = font;
    setFont(font);
    setMarginsFont(font);
    if (m_lexer)
        m_lexer->setFont(font);
}

void CodeEditor::applyFont(const QFont& font)
{
    m_defaultFont = font;
    setFont(font);
    setMarginsFont(font);
    if (m_lexer)
        m_lexer->setFont(font);
}

void CodeEditor::setScrollPastEnd(bool enabled)
{
    SendScintilla(SCI_SETSCROLLPASTEND, enabled ? 1 : 0);
}

void CodeEditor::setCodeFolding(bool enabled)
{
    if (enabled)
    {
        setFolding(QsciScintilla::BoxedTreeFoldStyle);
    }
    else
    {
        setFolding(QsciScintilla::NoFoldStyle);
    }
}

void CodeEditor::setWhitespaceVisible(bool visible)
{
    setWhitespaceVisibility(visible ? QsciScintilla::WsVisible : QsciScintilla::WsInvisible);
}

void CodeEditor::setupAutoCompletion(bool enabled, int threshold, bool caseSensitive)
{
    if (!enabled)
    {
        setAutoCompletionSource(QsciScintilla::AcsNone);
        SendScintilla(SCI_AUTOCSETAUTOMATIC, 0);
        return;
    }

    setAutoCompletionSource(QsciScintilla::AcsAll);
    setAutoCompletionThreshold(threshold);
    setAutoCompletionCaseSensitivity(caseSensitive);
    setAutoCompletionReplaceWord(true);
    setAutoCompletionShowSingle(false);
    setAutoCompletionFillupsEnabled(true);
    setAutoCompletionUseSingle(QsciScintilla::AcusExplicit);
    SendScintilla(SCI_AUTOCSETAUTOMATIC, 1);
}

void CodeEditor::setCursorStyle(CursorStyle style)
{
    int scintillaStyle = CARETSTYLE_LINE;
    switch (style)
    {
    case CursorStyle::Line:
        scintillaStyle = CARETSTYLE_LINE;
        break;
    case CursorStyle::Block:
        scintillaStyle = CARETSTYLE_BLOCK;
        break;
    case CursorStyle::Underline:
        scintillaStyle = CARETSTYLE_UNDERLINE;
        break;
    }
    SendScintilla(SCI_SETCARETSTYLE, scintillaStyle);
}

void CodeEditor::setCursorBlinking(bool enabled)
{
    int period = enabled ? 500 : 0;
    SendScintilla(SCI_SETCARETPERIOD, period);
}

void CodeEditor::setHighlightCurrentLine(bool enabled)
{
    setCaretLineVisible(enabled);
}

void CodeEditor::setVerticalEdge(bool enabled, int column)
{
    if (enabled)
    {
        SendScintilla(SCI_SETEDGEMODE, EDGE_LINE);
        SendScintilla(SCI_SETEDGECOLUMN, column);
        ThemeColors colors = getThemeColors(m_themeId);
        SendScintilla(SCI_SETEDGECOLOUR, colors.separator.red() | (colors.separator.green() << 8) | (colors.separator.blue() << 16));
    }
    else
    {
        SendScintilla(SCI_SETEDGEMODE, EDGE_NONE);
    }
}

void CodeEditor::setAutoCloseBrackets(bool enabled)
{
    m_autoCloseBrackets = enabled;
}

void CodeEditor::setReadOnlyMode(bool enabled)
{
    setReadOnly(enabled);
}

bool CodeEditor::isReadOnlyMode() const
{
    return isReadOnly();
}

void CodeEditor::toggleBookmark()
{
    int line = SendScintilla(SCI_LINEFROMPOSITION, SendScintilla(SCI_GETCURRENTPOS));
    toggleBookmark(line);
}

void CodeEditor::toggleBookmark(int line)
{
    if (hasBookmark(line))
    {
        markerDelete(line, MARKER_BOOKMARK);
    }
    else
    {
        markerAdd(line, MARKER_BOOKMARK);
    }
    emit bookmarksChanged();
}

bool CodeEditor::hasBookmark(int line) const
{
    return markersAtLine(line) != 0;
}

void CodeEditor::nextBookmark()
{
    int currentLine = SendScintilla(SCI_LINEFROMPOSITION, SendScintilla(SCI_GETCURRENTPOS));
    int lineCount = lines();
    int target = -1;

    for (int line = currentLine + 1; line < lineCount; ++line)
    {
        if (hasBookmark(line))
        {
            target = line;
            break;
        }
    }
    if (target == -1)
    {
        for (int line = 0; line <= currentLine; ++line)
        {
            if (hasBookmark(line))
            {
                target = line;
                break;
            }
        }
    }
    if (target != -1)
    {
        setCursorPosition(target, 0);
    }
}

void CodeEditor::prevBookmark()
{
    int currentLine = SendScintilla(SCI_LINEFROMPOSITION, SendScintilla(SCI_GETCURRENTPOS));
    int target = -1;

    for (int line = currentLine - 1; line >= 0; --line)
    {
        if (hasBookmark(line))
        {
            target = line;
            break;
        }
    }
    if (target == -1)
    {
        int lineCount = lines();
        for (int line = lineCount - 1; line > currentLine; --line)
        {
            if (hasBookmark(line))
            {
                target = line;
                break;
            }
        }
    }
    if (target != -1)
    {
        setCursorPosition(target, 0);
    }
}

void CodeEditor::clearBookmarks()
{
    markerDeleteAll(MARKER_BOOKMARK);
    emit bookmarksChanged();
}

QList<int> CodeEditor::bookmarkLines() const
{
    QList<int> lines;
    int lineCount = this->lines();
    for (int i = 0; i < lineCount; ++i)
    {
        if (markersAtLine(i) != 0)
        {
            lines.append(i);
        }
    }
    return lines;
}

void CodeEditor::setBookmarks(const QList<int>& lines)
{
    markerDeleteAll(MARKER_BOOKMARK);
    for (int line : lines)
    {
        if (line >= 0 && line < this->lines())
        {
            markerAdd(line, MARKER_BOOKMARK);
        }
    }
    emit bookmarksChanged();
}

CodeEditor::BracketContext CodeEditor::contextAtPosition(int pos) const
{
    BracketContext ctx;
    int line = SendScintilla(SCI_LINEFROMPOSITION, pos);
    int lineStart = SendScintilla(SCI_POSITIONFROMLINE, line);
    int lineLength = SendScintilla(SCI_GETLINEENDPOSITION, line);
    QString lineText = QsciScintilla::text(lineStart, lineLength);
    int col = pos - lineStart;

    for (int i = 0; i < col && i < lineText.length(); ++i)
    {
        QChar c = lineText[i];
        if (ctx.inComment)
            continue;
        if (c == '\\' && (ctx.inString || ctx.inCharLiteral))
        {
            ++i;
            continue;
        }
        if (c == '"' && !ctx.inCharLiteral)
            ctx.inString = !ctx.inString;
        else if (c == '\'' && !ctx.inString)
            ctx.inCharLiteral = !ctx.inCharLiteral;
        else if (c == '/' && i + 1 < lineText.length() && lineText[i + 1] == '/')
            ctx.inComment = true;
    }
    return ctx;
}

bool CodeEditor::handleAutoClose(QChar ch, int pos)
{
    static const struct
    {
        QChar open;
        QChar close;
    } pairs[] = {{'(', ')'}, {'[', ']'}, {'{', '}'}, {'"', '"'}, {'\'', '\''}};
    BracketContext ctx = contextAtPosition(pos);
    if (ctx.inComment || ctx.inCharLiteral)
        return false;

    for (const auto& pair : pairs)
    {
        if (ch != pair.open)
            continue;
        if (ch == '"' && ctx.inString)
            continue;
        if (ch == '\'' && ctx.inCharLiteral)
            continue;
        beginUndoAction();
        insert(QString(pair.open) + pair.close);
        SendScintilla(SCI_GOTOPOS, pos + 1);
        endUndoAction();
        return true;
    }
    return false;
}

bool CodeEditor::handleBracketSkip(QChar ch, int pos)
{
    static const QChar closers[] = {')', ']', '}', '"', '\''};
    BracketContext ctx = contextAtPosition(pos);
    if (ctx.inString || ctx.inComment)
        return false;

    for (QChar closer : closers)
    {
        if (ch != closer)
            continue;
        int nextPos = pos + 1;
        if (nextPos >= length())
            return false;
        int nextChar = SendScintilla(SCI_GETCHARAT, nextPos);
        if (nextChar == static_cast<int>(closer.toLatin1()))
        {
            SendScintilla(SCI_GOTOPOS, nextPos);
            return true;
        }
    }
    return false;
}

void CodeEditor::keyPressEvent(QKeyEvent* event)
{
    // Handle snippet mode Tab/Shift+Tab navigation
    if (m_snippetActive)
    {
        if (event->key() == Qt::Key_Tab && !(event->modifiers() & ~Qt::ShiftModifier))
        {
            if (event->modifiers() & Qt::ShiftModifier)
            {
                retreatTabStop();
            }
            else
            {
                advanceTabStop();
            }
            event->accept();
            return;
        }
        if (event->key() == Qt::Key_Escape)
        {
            exitSnippetMode();
            event->accept();
            return;
        }
    }

    if (m_autoCloseBrackets && !isReadOnly() && !(event->modifiers() & (Qt::ControlModifier | Qt::AltModifier | Qt::MetaModifier)))
    {
        QString eventText = event->text();
        if (!eventText.isEmpty())
        {
            QChar ch = eventText.at(0);
            int pos = SendScintilla(SCI_GETCURRENTPOS);

            if (handleAutoClose(ch, pos) || handleBracketSkip(ch, pos))
            {
                event->accept();
                return;
            }
        }
    }
    QsciScintilla::keyPressEvent(event);
}

// ── Snippet Implementation ──────────────────────────────────────────────────

void CodeEditor::insertSnippet(const Snippet& snippet)
{
    if (isReadOnly())
        return;

    Snippet::ExpandedSnippet expanded = snippet.expand();
    int pos = SendScintilla(SCI_GETCURRENTPOS);
    enterSnippetMode(expanded, pos);
}

void CodeEditor::enterSnippetMode(const Snippet::ExpandedSnippet& expanded, int insertPos, const QString& triggerText)
{
    // If we were already in snippet mode, clean up
    if (m_snippetActive)
        clearSnippetMarkers();

    QString textToInsert = expanded.text;

    // Replace trigger text if present (used for predictive snippets)
    if (!triggerText.isEmpty())
    {
        int triggerStart = insertPos - triggerText.length();
        if (triggerStart >= 0)
        {
            SendScintilla(SCI_SETSELECTIONSTART, triggerStart);
            SendScintilla(SCI_SETSELECTIONEND, insertPos);
            insertPos = triggerStart;
        }
    }

    // Insert the expanded snippet text
    beginUndoAction();

    // Replace current selection if any
    int selStart = SendScintilla(SCI_GETSELECTIONSTART);
    int selEnd = SendScintilla(SCI_GETSELECTIONEND);
    if (selStart != selEnd)
    {
        if (triggerText.isEmpty())
        {
            insertPos = selStart;
            SendScintilla(SCI_SETSELECTIONSTART, selStart);
            SendScintilla(SCI_SETSELECTIONEND, selEnd);
        }
    }

    insert(textToInsert);
    endUndoAction();

    m_snippetStartPos = insertPos;
    m_snippetEndPos = insertPos + textToInsert.length();

    // Set up tab stops with absolute positions
    m_tabStopInfos.clear();
    for (const auto& ts : expanded.tabStops)
    {
        for (int offset : ts.positions)
        {
            TabStopInfo info;
            info.number = ts.number;
            info.pos = insertPos + offset;
            info.length = ts.length;
            info.defaultValue = ts.defaultValue;
            m_tabStopInfos.append(info);
        }
    }

    // Sort by position
    std::sort(m_tabStopInfos.begin(), m_tabStopInfos.end(),
              [](const TabStopInfo& a, const TabStopInfo& b) { return a.pos < b.pos; });

    if (m_tabStopInfos.isEmpty())
    {
        // No tab stops - just position cursor at end of snippet
        SendScintilla(SCI_SETCURRENTPOS, m_snippetEndPos);
        SendScintilla(SCI_SETANCHOR, m_snippetEndPos);
        m_snippetActive = false;
        emit snippetModeChanged(false);
        return;
    }

    // Mark all tab stops with invisible indicators
    for (const auto& info : m_tabStopInfos)
    {
        if (info.length > 0 || info.defaultValue.isEmpty())
        {
            int len = qMax(info.length, 1);
            SendScintilla(SCI_INDICATORFILLRANGE, static_cast<unsigned long>(info.pos), static_cast<unsigned long>(len));
        }
    }

    m_snippetActive = true;
    m_currentTabStopIdx = 0;
    emit snippetModeChanged(true);

    // Select first tab stop
    selectTabStopRange(m_tabStopInfos[0].pos, m_tabStopInfos[0].length);
}

void CodeEditor::advanceTabStop()
{
    if (!m_snippetActive || m_tabStopInfos.isEmpty())
        return;

    // Find next unique tab stop number
    int currentNumber = m_tabStopInfos[m_currentTabStopIdx].number;
    int nextIdx = -1;

    // Find the first tab stop with a number > current
    for (int i = 0; i < m_tabStopInfos.size(); ++i)
    {
        if (m_tabStopInfos[i].number > currentNumber)
        {
            nextIdx = i;
            break;
        }
    }

    if (nextIdx < 0)
    {
        // No more tab stops - look for $0 (final position, number 0)
        // If no $0, go to end of snippet
        SendScintilla(SCI_SETCURRENTPOS, m_snippetEndPos);
        SendScintilla(SCI_SETANCHOR, m_snippetEndPos);
        exitSnippetMode();
        return;
    }

    m_currentTabStopIdx = nextIdx;
    selectTabStopRange(m_tabStopInfos[nextIdx].pos, m_tabStopInfos[nextIdx].length);
}

void CodeEditor::retreatTabStop()
{
    if (!m_snippetActive || m_tabStopInfos.isEmpty())
        return;

    // Find previous unique tab stop number
    int currentNumber = m_tabStopInfos[m_currentTabStopIdx].number;
    int prevIdx = -1;

    for (int i = m_tabStopInfos.size() - 1; i >= 0; --i)
    {
        if (m_tabStopInfos[i].number < currentNumber)
        {
            prevIdx = i;
            break;
        }
    }

    if (prevIdx < 0)
    {
        // Go to start of snippet
        SendScintilla(SCI_SETCURRENTPOS, m_snippetStartPos);
        SendScintilla(SCI_SETANCHOR, m_snippetStartPos);
        return;
    }

    m_currentTabStopIdx = prevIdx;
    selectTabStopRange(m_tabStopInfos[prevIdx].pos, m_tabStopInfos[prevIdx].length);
}

void CodeEditor::exitSnippetMode()
{
    if (!m_snippetActive)
        return;

    clearSnippetMarkers();
    m_snippetActive = false;
    m_currentTabStopIdx = -1;
    m_tabStopInfos.clear();
    emit snippetModeChanged(false);
}

void CodeEditor::clearSnippetMarkers()
{
    if (m_snippetEndPos > m_snippetStartPos)
    {
        SendScintilla(SCI_INDICATORCLEARRANGE, static_cast<unsigned long>(m_snippetStartPos), static_cast<unsigned long>(m_snippetEndPos - m_snippetStartPos));
    }
}

void CodeEditor::selectTabStopRange(int pos, int len)
{
    if (len > 0)
    {
        SendScintilla(SCI_SETSELECTIONSTART, pos);
        SendScintilla(SCI_SETSELECTIONEND, pos + len);
    }
    else
    {
        SendScintilla(SCI_SETCURRENTPOS, pos);
        SendScintilla(SCI_SETANCHOR, pos);
    }
}

void CodeEditor::recalculateTabStopPositions()
{
    // Re-read tab stop positions using indicators
    for (int i = 0; i < m_tabStopInfos.size(); ++i)
    {
        auto& info = m_tabStopInfos[i];
        int start = static_cast<int>(SendScintilla(SCI_INDICATORSTART, static_cast<unsigned long>(SNIPPET_INDICATOR), static_cast<unsigned long>(info.pos)));
        int end = static_cast<int>(SendScintilla(SCI_INDICATOREND, static_cast<unsigned long>(SNIPPET_INDICATOR), static_cast<unsigned long>(info.pos)));
        if (start >= 0 && end > start)
        {
            info.pos = start;
            info.length = end - start;
        }
    }
}


