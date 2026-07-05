#include "codeeditor.h"

#include "languageinfo.h"
#include "settingsmanager.h"
#include "snippetmanager.h"
#include "theme.h"
#include "lsp/lspclient.h"
#include "lsp/lspservermanager.h"
#include "lsp/lsptypes.h"
#include "lsp/lspindicators.h"
#include <QApplication>
#include <QClipboard>
#include <QContextMenuEvent>
#include <QDragEnterEvent>
#include <QDropEvent>
#include <QFileInfo>
#include <QFont>
#include <QIcon>
#include <QInputDialog>
#include <QKeyEvent>
#include <QMenu>
#include <QMimeData>
#include <QTimer>
#include <QToolTip>

#include <Qsci/qsciapis.h>

namespace
{
constexpr int SCI_SETSCROLLPASTEND = 2694;
constexpr int SCI_SETCARETSTYLE = 2068;
constexpr int SCI_SETCARETPERIOD = 2075;
constexpr int SCI_GETCHARAT = 2007;
constexpr int SCI_GETCURRENTPOS = 2008;
constexpr int SCI_AUTOCSETAUTOMATIC = 2231;
constexpr int SCI_INDICATORSTART = 2102;
constexpr int SCI_INDICATOREND = 2103;
constexpr int SCI_SETMOUSEDWELLTIME = 2264;
constexpr int SCI_SETSAVEPOINT = 2172;
constexpr int SCI_COLUMNTOBYPOS = 2616;

constexpr int INDIC_HIDDEN = 8;

constexpr int CARETSTYLE_LINE = 1;
constexpr int CARETSTYLE_BLOCK = 2;
constexpr int CARETSTYLE_UNDERLINE = 4;

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
    indicatorDefine(QsciScintilla::HiddenIndicator, SNIPPET_INDICATOR);

    // Set up LSP diagnostic indicators
    setupLspIndicators();

    // Set up LSP timers
    m_completionTimer = new QTimer(this);
    m_completionTimer->setSingleShot(true);
    m_completionTimer->setInterval(200);
    connect(m_completionTimer, &QTimer::timeout, this, [this]() {
        if (m_lspActive && m_lspManager)
            triggerCompletion();
    });

    m_diagnosticsTimer = new QTimer(this);
    m_diagnosticsTimer->setSingleShot(true);
    m_diagnosticsTimer->setInterval(500);
    connect(m_diagnosticsTimer, &QTimer::timeout, this, [this]() {
        if (m_lspActive)
            sendDidChange();
    });

    // Highlight timer: debounces document highlight requests on cursor move
    m_highlightTimer = new QTimer(this);
    m_highlightTimer->setSingleShot(true);
    m_highlightTimer->setInterval(300);
    connect(m_highlightTimer, &QTimer::timeout, this, [this]() {
        requestDocumentHighlight();
    });

    // Trigger document highlights when cursor position changes
    connect(this, &QsciScintilla::cursorPositionChanged, this, [this](int, int) {
        if (m_lspActive)
            m_highlightTimer->start();
    });

    // Connect text change signal to trigger diagnostics for paste/undo/redo etc.
    connect(this, &QsciScintilla::textChanged, this, [this]() {
        if (m_lspActive)
            m_diagnosticsTimer->start();
    });

    // Enable mouse dwell timer for LSP hover tooltips (500ms)
    SendScintilla(SCI_SETMOUSEDWELLTIME, 500);

    // Connect mouse dwell for hover
    connect(this, static_cast<void(QsciScintillaBase::*)(int, int, int)>(&QsciScintillaBase::SCN_DWELLSTART),
            this, [this](int position, int, int) {
        if (!m_lspActive || !m_lspManager || m_fileName.isEmpty())
            return;

        QString uri = lsp::uriFromPath(m_fileName);
        auto* client = m_lspManager->clientForUri(uri);
        if (!client || !client->capabilities().hoverProvider)
            return;

        int line, col;
        lineIndexFromPosition(position, &line, &col);
        lsp::Position pos{line, col};
        client->requestHover(uri, pos);
    });

    connect(this, static_cast<void(QsciScintillaBase::*)(int, int, int)>(&QsciScintillaBase::SCN_DWELLEND),
            this, [this](int, int, int) {
        QToolTip::hideText();
    });

    // Connect char added signal for LSP completion trigger
    connect(this, static_cast<void(QsciScintillaBase::*)(int)>(&QsciScintillaBase::SCN_CHARADDED), this, &CodeEditor::onCharAdded);

    // Connect autocompletion selection signal to handle snippet expansion
    connect(this, static_cast<void(QsciScintillaBase::*)(const char*, int)>(&QsciScintillaBase::SCN_AUTOCSELECTION), this, [this](const char* selection, int position) {
        if (!selection)
            return;

        QString selectedText = QString::fromUtf8(selection);

        // Handle snippet expansion
        if (SettingsManager::instance().predictiveSnippets() && selectedText.endsWith(SNIPPET_MARKER))
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

        // Handle LSP completion item with custom insertText / textEdit
        if (!m_lspCompletionItems.isEmpty())
        {
            // Extract label by removing detail suffix (separated by " ?")
            QString label = selectedText;
            int sepIdx = label.indexOf(QStringLiteral(" ?"));
            if (sepIdx >= 0)
                label = label.left(sepIdx);

            for (const auto& item : m_lspCompletionItems)
            {
                if (item.label == label && item.insertText != label)
                {
                    int startPos = position - label.length();
                    if (startPos >= 0)
                    {
                        int startLine, startCol, endLine, endCol;
                        lineIndexFromPosition(startPos, &startLine, &startCol);
                        lineIndexFromPosition(position, &endLine, &endCol);
                        setSelection(startLine, startCol, endLine, endCol);
                        replaceSelectedText(item.insertText);
                    }
                    break;
                }
            }
        }
    });
}

CodeEditor::~CodeEditor()
{
    // Detach lexer from QsciScintilla before QScopedPointer destroys it,
    // preventing use-after-free in ~QsciScintilla.
    setLexer(nullptr);
}

void CodeEditor::dragEnterEvent(QDragEnterEvent* event)
{
    if (event->mimeData()->hasFormat("application/x-devpad-tab"))
        return;

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
    if (event->mimeData()->hasFormat("application/x-devpad-tab"))
        return;

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
    {
        QFontMetrics fm(m_defaultFont);
        setMarginWidth(BOOKMARK_MARGIN, fm.height());
    }
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
        setEdgeMode(QsciScintilla::EdgeLine);
        setEdgeColor(colors.separator);
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
    setLexer(nullptr);
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
    int width = fm.horizontalAdvance(QString(digits, '9')) + fm.height() / 2;
    setMarginWidth(0, width);
}

void CodeEditor::forceModified()
{
    int line, index;
    getCursorPosition(&line, &index);

    int endPos = length();
    int endLine, endCol;
    lineIndexFromPosition(endPos, &endLine, &endCol);
    insertAt(QStringLiteral(" "), endLine, endCol);
    // Mark saved after insert so the delete becomes a net change
    SendScintilla(SCI_SETSAVEPOINT);
    setSelection(endLine, endCol, endLine, endCol + 1);
    replaceSelectedText(QString());

    // Restore cursor — deleting back moved it
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
        setEdgeMode(QsciScintilla::EdgeLine);
        setEdgeColumn(column);
        ThemeColors colors = getThemeColors(m_themeId);
        setEdgeColor(colors.separator);
    }
    else
    {
        setEdgeMode(QsciScintilla::EdgeNone);
    }
}

void CodeEditor::setAutoCloseBrackets(bool enabled)
{
    m_autoCloseBrackets = enabled;
}

void CodeEditor::setupLspIndicators()
{
    // Error indicator: red squiggle
    indicatorDefine(QsciScintilla::SquiggleIndicator, lsp::LSP_INDICATOR_ERROR);
    setIndicatorForegroundColor(QColor(255, 0, 0), lsp::LSP_INDICATOR_ERROR);

    // Warning indicator: orange squiggle
    indicatorDefine(QsciScintilla::SquiggleIndicator, lsp::LSP_INDICATOR_WARNING);
    setIndicatorForegroundColor(QColor(255, 165, 0), lsp::LSP_INDICATOR_WARNING);

    // Info indicator: blue squiggle
    indicatorDefine(QsciScintilla::SquiggleIndicator, lsp::LSP_INDICATOR_INFO);
    setIndicatorForegroundColor(QColor(0, 170, 255), lsp::LSP_INDICATOR_INFO);

    // Semantic token indicator: gray squiggle
    indicatorDefine(QsciScintilla::SquiggleIndicator, lsp::LSP_INDICATOR_SEMANTIC);
    setIndicatorForegroundColor(QColor(180, 180, 180), lsp::LSP_INDICATOR_SEMANTIC);

    // Highlight indicator: dark yellow squiggle (for document highlights)
    indicatorDefine(QsciScintilla::SquiggleIndicator, lsp::LSP_INDICATOR_HIGHLIGHT);
    setIndicatorForegroundColor(QColor(200, 180, 0), lsp::LSP_INDICATOR_HIGHLIGHT);

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
    int line, col;
    getCursorPosition(&line, &col);
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
    return (markersAtLine(line) & (1 << MARKER_BOOKMARK)) != 0;
}

void CodeEditor::nextBookmark()
{
    int currentLine, col;
    getCursorPosition(&currentLine, &col);
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
    int currentLine, col;
    getCursorPosition(&currentLine, &col);
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
        if ((markersAtLine(i) & (1 << MARKER_BOOKMARK)) != 0)
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

void CodeEditor::toggleComment()
{
    const auto* cs = commentSyntaxForLanguage(m_syntax);
    if (!cs)
        return;

    struct UndoGuard {
        QsciScintilla& ed;
        ~UndoGuard() { ed.endUndoAction(); }
    } guard{*this};
    beginUndoAction();

    int lineFrom, colFrom, lineTo, colTo;
    getSelection(&lineFrom, &colFrom, &lineTo, &colTo);
    bool hasSel = hasSelectedText();

    // Block comment: unwrap if selection is block-commented
    if (hasSel && !cs->blockCommentStart.isEmpty()) {
        QString sel = selectedText();
        if (sel.startsWith(cs->blockCommentStart) && sel.endsWith(cs->blockCommentEnd)) {
            int innerLen = sel.length() - cs->blockCommentStart.length() - cs->blockCommentEnd.length();
            replaceSelectedText(sel.mid(cs->blockCommentStart.length(), innerLen));
            return;
        }
    }

    // If no line comment token, nothing more to do
    if (cs->lineComment.isEmpty()) {
        return;
    }

    // Determine line range
    if (hasSel) {
        // Block comment wrap for a single-line selection
        if (!cs->blockCommentStart.isEmpty() && lineFrom == lineTo) {
            QString sel = selectedText();
            replaceSelectedText(cs->blockCommentStart + sel + cs->blockCommentEnd);
            return;
        }
        if (colTo == 0 && lineTo > lineFrom)
            lineTo--;
    } else {
        getCursorPosition(&lineFrom, &colFrom);
        lineTo = lineFrom;
    }

    // Build full text of the range (text(line) includes trailing \n)
    QString fullText;
    for (int line = lineFrom; line <= lineTo; ++line)
        fullText += text(line);

    QStringList lines = fullText.split('\n');
    // Remove trailing empty element from final \n
    if (!lines.isEmpty() && lines.last().isEmpty())
        lines.removeLast();

    // Determine mode based on first non-empty line (VS Code behavior)
    int commentLen = cs->lineComment.length();
    bool shouldComment = true;
    for (const QString &ln : lines) {
        if (ln.isEmpty())
            continue;
        int firstNonSpace = 0;
        while (firstNonSpace < ln.length() && ln[firstNonSpace] == ' ')
            firstNonSpace++;
        shouldComment = !ln.mid(firstNonSpace).startsWith(cs->lineComment);
        break;
    }

    // Apply
    QStringList result;
    for (const QString &ln : lines) {
        if (ln.isEmpty()) {
            result << ln;
            continue;
        }
        int firstNonSpace = 0;
        while (firstNonSpace < ln.length() && ln[firstNonSpace] == ' ')
            firstNonSpace++;

        QString modified = ln;
        if (shouldComment) {
            modified.insert(firstNonSpace, cs->lineComment);
        } else {
            modified.remove(firstNonSpace, commentLen);
        }
        result << modified;
    }

    QString joined = result.join('\n');
    joined += '\n';
    int lastLineLen = text(lineTo).length();
    setSelection(lineFrom, 0, lineTo, lastLineLen);
    replaceSelectedText(joined);
}

CodeEditor::BracketContext CodeEditor::contextAtPosition(int pos) const
{
    BracketContext ctx;
    if (pos <= 0)
        return ctx;

    // Fast path: use Scintilla styling to determine context at position
    // If the character before pos has default style (0), we're in code
    int prevStyle = static_cast<int>(SendScintilla(SCI_GETSTYLEAT, pos - 1));
    if (prevStyle == 0) {
        // Quick check: if prev char is a line comment opener, we're in a comment
        int line = SendScintilla(SCI_LINEFROMPOSITION, pos);
        int lineStart = SendScintilla(SCI_POSITIONFROMLINE, line);
        int col = pos - lineStart;
        if (col >= 2) {
            char c1 = static_cast<char>(SendScintilla(SCI_GETCHARAT, pos - 2));
            char c2 = static_cast<char>(SendScintilla(SCI_GETCHARAT, pos - 1));
            if (c1 == '/' && c2 == '/') {
                ctx.inComment = true;
                return ctx;
            }
        }
        // Default style means we're in plain code — no comment/string context
        return ctx;
    }

    // Slow path: scan backwards to determine context using style categories
    // Check the style at pos-1 against known comment and string ranges
    // For QScintilla lexers, styles above 0 are categorized as:
    //   Comment styles: typically 1-3
    //   String styles: typically 6-7 (but varies by lexer)
    // We use a bounded scan for block comment detection as fallback

    int line = SendScintilla(SCI_LINEFROMPOSITION, pos);
    int col = pos - SendScintilla(SCI_POSITIONFROMLINE, line);

    // Bounded scan: at most 500 lines back for block comment detection
    constexpr int MAX_SCAN_LINES = 500;
    int scanStart = std::max(0, line - MAX_SCAN_LINES);

    bool inBlockComment = false;
    for (int l = scanStart; l < line; ++l)
    {
        int lStart = SendScintilla(SCI_POSITIONFROMLINE, l);
        int lEnd = SendScintilla(SCI_GETLINEENDPOSITION, l);
        QString lText = QsciScintilla::text(lStart, lEnd);

        for (int i = 0; i < lText.length(); ++i)
        {
            QChar c = lText[i];
            if (!inBlockComment)
            {
                if (c == '/' && i + 1 < lText.length() && lText[i + 1] == '*')
                {
                    inBlockComment = true;
                    ++i;
                }
            }
            else
            {
                if (c == '*' && i + 1 < lText.length() && lText[i + 1] == '/')
                {
                    inBlockComment = false;
                    ++i;
                }
            }
        }
    }

    // Scan current line up to the cursor position
    int lineStart = SendScintilla(SCI_POSITIONFROMLINE, line);
    int lineLength = SendScintilla(SCI_GETLINEENDPOSITION, line);
    QString lineText = QsciScintilla::text(lineStart, lineLength);

    for (int i = 0; i < col && i < lineText.length(); ++i)
    {
        QChar c = lineText[i];
        if (inBlockComment)
        {
            if (c == '*' && i + 1 < lineText.length() && lineText[i + 1] == '/')
            {
                inBlockComment = false;
                ++i;
            }
            continue;
        }
        if (ctx.inComment)
            continue;
        if (c == '/' && i + 1 < lineText.length())
        {
            if (lineText[i + 1] == '*')
            {
                inBlockComment = true;
                ++i;
                continue;
            }
            if (lineText[i + 1] == '/')
            {
                ctx.inComment = true;
                continue;
            }
        }
        if (c == '\\' && (ctx.inString || ctx.inCharLiteral))
        {
            ++i;
            continue;
        }
        if (c == '"' && !ctx.inCharLiteral)
            ctx.inString = !ctx.inString;
        else if (c == '\'' && !ctx.inString)
            ctx.inCharLiteral = !ctx.inCharLiteral;
    }

    ctx.inBlockComment = inBlockComment;
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
    if (ctx.inComment || ctx.inBlockComment || ctx.inCharLiteral)
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
        int l, c;
        lineIndexFromPosition(pos + 1, &l, &c);
        setCursorPosition(l, c);
        endUndoAction();
        return true;
    }
    return false;
}

bool CodeEditor::handleBracketSkip(QChar ch, int pos)
{
    static const QChar closers[] = {')', ']', '}', '"', '\''};
    BracketContext ctx = contextAtPosition(pos);

    // Never skip brackets inside comments
    if (ctx.inComment || ctx.inBlockComment)
        return false;

    bool isQuote = (ch == '"' || ch == '\'');
    if (isQuote)
    {
        // Inside a string, typing the matching quote should skip the next one
        // to end the string without inserting a duplicate
        bool inRelevantString = (ch == '"' && ctx.inString) || (ch == '\'' && ctx.inCharLiteral);
        if (!inRelevantString)
            return false;
    }
    else
    {
        // For non-quote brackets, no skipping inside strings
        if (ctx.inString || ctx.inCharLiteral)
            return false;
    }

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
            int nl, nc;
            lineIndexFromPosition(nextPos, &nl, &nc);
            setCursorPosition(nl, nc);
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
    // Ctrl+/: toggle comment (Scintilla consumes the event before QAction shortcuts)
    if (!isReadOnly() && (event->modifiers() & Qt::ControlModifier) && event->key() == Qt::Key_Slash) {
        toggleComment();
        event->accept();
        return;
    }

    QsciScintilla::keyPressEvent(event);
}

// ── Snippet Implementation ──────────────────────────────────────────────────

void CodeEditor::insertSnippet(const Snippet& snippet)
{
    if (isReadOnly())
        return;

    Snippet::ExpandedSnippet expanded = snippet.expand();
    int line, col;
    getCursorPosition(&line, &col);
    int pos = positionFromLineIndex(line, col);
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
            int tl, tc, il, ic;
            lineIndexFromPosition(triggerStart, &tl, &tc);
            lineIndexFromPosition(insertPos, &il, &ic);
            setSelection(tl, tc, il, ic);
            insertPos = triggerStart;
        }
    }

    // Insert the expanded snippet text
    beginUndoAction();

    // Replace current selection if any
    if (hasSelectedText())
    {
        if (triggerText.isEmpty())
        {
            int selLineFrom, selColFrom, selLineTo, selColTo;
            getSelection(&selLineFrom, &selColFrom, &selLineTo, &selColTo);
            insertPos = positionFromLineIndex(selLineFrom, selColFrom);
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
        int el, ec;
        lineIndexFromPosition(m_snippetEndPos, &el, &ec);
        setCursorPosition(el, ec);
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
            int lf, cf, lt, ct;
            lineIndexFromPosition(info.pos, &lf, &cf);
            lineIndexFromPosition(info.pos + len, &lt, &ct);
            fillIndicatorRange(lf, cf, lt, ct, SNIPPET_INDICATOR);
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
        int el, ec;
        lineIndexFromPosition(m_snippetEndPos, &el, &ec);
        setCursorPosition(el, ec);
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
        int sl, sc;
        lineIndexFromPosition(m_snippetStartPos, &sl, &sc);
        setCursorPosition(sl, sc);
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
        int lf, cf, lt, ct;
        lineIndexFromPosition(m_snippetStartPos, &lf, &cf);
        lineIndexFromPosition(m_snippetEndPos, &lt, &ct);
        clearIndicatorRange(lf, cf, lt, ct, SNIPPET_INDICATOR);
    }
}

void CodeEditor::selectTabStopRange(int pos, int len)
{
    if (len > 0)
    {
        int lf, cf, lt, ct;
        lineIndexFromPosition(pos, &lf, &cf);
        lineIndexFromPosition(pos + len, &lt, &ct);
        setSelection(lf, cf, lt, ct);
    }
    else
    {
        int l, c;
        lineIndexFromPosition(pos, &l, &c);
        setCursorPosition(l, c);
    }
}

void CodeEditor::recalculateTabStopPositions()
{
    // Re-read tab stop positions using indicators (no QScintilla equivalent for indicator queries)
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

void CodeEditor::contextMenuEvent(QContextMenuEvent* event)
{
    QMenu menu(this);

    bool hasSelection = !selectedText().isEmpty();
    bool canUndo = isUndoAvailable();
    bool canRedo = isRedoAvailable();
    bool readOnly = isReadOnly();

    QAction* undoAct = menu.addAction(QIcon(":/icons/Edit/undo.svg"), tr("Undo"));
    undoAct->setShortcut(QKeySequence::Undo);
    undoAct->setEnabled(canUndo && !readOnly);

    QAction* redoAct = menu.addAction(QIcon(":/icons/Edit/redo.svg"), tr("Redo"));
    redoAct->setShortcut(QKeySequence::Redo);
    redoAct->setEnabled(canRedo && !readOnly);

    menu.addSeparator();

    QAction* cutAct = menu.addAction(QIcon(":/icons/Edit/cut.svg"), tr("Cut"));
    cutAct->setShortcut(QKeySequence::Cut);
    cutAct->setEnabled(hasSelection && !readOnly);

    QAction* copyAct = menu.addAction(QIcon(":/icons/Edit/copy.svg"), tr("Copy"));
    copyAct->setShortcut(QKeySequence::Copy);
    copyAct->setEnabled(hasSelection);

    QAction* pasteAct = menu.addAction(QIcon(":/icons/Edit/paste.svg"), tr("Paste"));
    pasteAct->setShortcut(QKeySequence::Paste);
    pasteAct->setEnabled(!readOnly);

    QAction* deleteAct = menu.addAction(QIcon(":/icons/Edit/delete.svg"), tr("Delete"));
    deleteAct->setShortcut(QKeySequence::Delete);
    deleteAct->setEnabled(hasSelection && !readOnly);

    menu.addSeparator();

    QAction* selectAllAct = menu.addAction(QIcon(":/icons/Edit/selectall.svg"), tr("Select All"));
    selectAllAct->setShortcut(QKeySequence::SelectAll);

    menu.addSeparator();

    QAction* findAct = menu.addAction(QIcon(":/icons/Edit/find.svg"), tr("Find..."));
    findAct->setShortcut(QKeySequence::Find);

    QAction* replaceAct = menu.addAction(QIcon(":/icons/Edit/replace.svg"), tr("Replace..."));
    replaceAct->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_H));

    QAction* goToLineAct = menu.addAction(QIcon(":/icons/Edit/goto.svg"), tr("Go To Line..."));
    goToLineAct->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_G));

    menu.addSeparator();

    QMenu* bookmarksMenu = menu.addMenu(QIcon(":/icons/Edit/bookmarks.svg"), tr("Bookmarks"));

    QAction* toggleBookmarkAct = bookmarksMenu->addAction(QIcon(":/icons/Edit/togglebookmark.svg"), tr("Toggle Bookmark"));
    toggleBookmarkAct->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_F2));
    toggleBookmarkAct->setEnabled(!readOnly);

    QAction* nextBookmarkAct = bookmarksMenu->addAction(QIcon(":/icons/Edit/bookmarknext.svg"), tr("Next Bookmark"));
    nextBookmarkAct->setShortcut(QKeySequence(Qt::Key_F2));

    QAction* prevBookmarkAct = bookmarksMenu->addAction(QIcon(":/icons/Edit/bookmarkprevious.svg"), tr("Previous Bookmark"));
    prevBookmarkAct->setShortcut(QKeySequence(Qt::SHIFT | Qt::Key_F2));

    bookmarksMenu->addSeparator();

    QAction* clearBookmarksAct = bookmarksMenu->addAction(QIcon(":/icons/Common/clear.svg"), tr("Clear All Bookmarks"));
    clearBookmarksAct->setShortcut(QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_F2));

    menu.addSeparator();

    QAction* insertSnippetAct = menu.addAction(QIcon(":/icons/Edit/insertsnippet.svg"), tr("Insert Snippet..."));
    insertSnippetAct->setShortcut(QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_I));
    insertSnippetAct->setEnabled(!readOnly);

    menu.addSeparator();

    QAction* toggleCommentAct = menu.addAction(QIcon(":/icons/Edit/togglecomment.svg"), tr("Toggle Comment"));
    toggleCommentAct->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_Slash));
    toggleCommentAct->setEnabled(!readOnly);

    QAction* chosen = menu.exec(event->globalPos());

    if (!chosen)
        return;

    if (chosen == undoAct)
        undo();
    else if (chosen == redoAct)
        redo();
    else if (chosen == cutAct)
        cut();
    else if (chosen == copyAct)
        copy();
    else if (chosen == pasteAct)
        paste();
    else if (chosen == deleteAct)
        removeSelectedText();
    else if (chosen == selectAllAct)
        selectAll();
    else if (chosen == findAct)
        emit findRequested();
    else if (chosen == replaceAct)
        emit replaceRequested();
    else if (chosen == goToLineAct)
        emit goToLineRequested();
    else if (chosen == toggleBookmarkAct)
        toggleBookmark();
    else if (chosen == nextBookmarkAct)
        nextBookmark();
    else if (chosen == prevBookmarkAct)
        prevBookmark();
    else if (chosen == clearBookmarksAct)
        clearBookmarks();
    else if (chosen == insertSnippetAct)
        emit insertSnippetRequested();
    else if (chosen == toggleCommentAct)
        toggleComment();
}

// ── LSP Integration ─────────────────────────────────────────────

void CodeEditor::setLspServerManager(lsp::LspServerManager* manager)
{
    m_lspManager = manager;
}

void CodeEditor::setLspLanguage(const QString& language)
{
    m_lspLanguage = language;
}

void CodeEditor::onCharAdded(int charadded)
{
    if (!m_lspActive || !m_lspManager)
        return;

    QChar ch(charadded);
    m_lastTriggerChar = charadded;
    m_lastCharWasTrigger = false;

    // Detect trigger characters from server capabilities
    QString uri = lsp::uriFromPath(m_fileName);
    auto* client = m_lspManager ? m_lspManager->clientForUri(uri) : nullptr;
    if (client && client->capabilities().completionProvider) {
        for (const auto& tc : client->capabilities().completionTriggerChars) {
            if (!tc.isEmpty() && QChar(tc[0]) == ch) {
                m_lastCharWasTrigger = true;
                break;
            }
        }
    }

    // Trigger completion on alphanumeric characters and trigger characters
    if (m_lastCharWasTrigger || ch.isLetterOrNumber() || ch == QLatin1Char('.') || ch == QLatin1Char('>') ||
        ch == QLatin1Char(':') || ch == QLatin1Char('/') || ch == QLatin1Char('-') ||
        ch == QLatin1Char('_')) {
        int threshold = SettingsManager::instance().lspCompletionTriggerChars();
        // Count characters since last non-alphanumeric
        int pos = SendScintilla(SCI_GETCURRENTPOS);
        int count = 0;
        for (int i = pos - 1; i >= 0 && count < threshold; i--) {
            char c = static_cast<char>(SendScintilla(SCI_GETCHARAT, i));
            if (std::isalnum(c) || c == '_')
                count++;
            else
                break;
        }
        if (m_lastCharWasTrigger || count >= threshold) {
            m_completionTimer->start();
        }
    } else if (ch == QLatin1Char('(')) {
        // Request signature help on open paren
        QString uri = lsp::uriFromPath(m_fileName);
        int line, col;
        getCursorPosition(&line, &col);
        lsp::Position pos{line, qMax(0, col - 1)};
        auto* client = m_lspManager->clientForUri(uri);
        if (client)
            client->requestSignatureHelp(uri, pos);
    }

    // Trigger diagnostics after typing stops
    m_diagnosticsTimer->start();
}

void CodeEditor::sendDidOpen()
{
    if (!m_lspManager || m_fileName.isEmpty() || m_lspLanguage.isEmpty())
        return;

    QString uri = lsp::uriFromPath(m_fileName);
    m_lspManager->openDocument(m_lspLanguage, uri, text());
    m_lspActive = true;
}

void CodeEditor::sendDidChange()
{
    if (!m_lspManager || m_fileName.isEmpty() || !m_lspActive)
        return;

    QString uri = lsp::uriFromPath(m_fileName);
    m_docVersion++;
    m_lspManager->changeDocument(uri, text());
}

void CodeEditor::triggerCompletion()
{
    if (!m_lspActive || !m_lspManager || m_fileName.isEmpty())
        return;

    QString uri = lsp::uriFromPath(m_fileName);
    auto* client = m_lspManager->clientForUri(uri);
    if (!client || !client->capabilities().completionProvider)
        return;

    int line, col;
    getCursorPosition(&line, &col);
    lsp::Position pos{line, col};

    int triggerKind = m_lastCharWasTrigger ? 2 : 1;
    m_lastCharWasTrigger = false;
    client->requestCompletion(uri, pos, triggerKind);
}

void CodeEditor::goToDefinition()
{
    if (!m_lspActive || !m_lspManager || m_fileName.isEmpty())
        return;

    QString uri = lsp::uriFromPath(m_fileName);
    auto* client = m_lspManager->clientForUri(uri);
    if (!client || !client->capabilities().definitionProvider)
        return;

    int line, col;
    getCursorPosition(&line, &col);
    lsp::Position pos{line, col};

    QString filePath = m_fileName;
    client->requestDefinition(uri, pos);
}

void CodeEditor::goToTypeDefinition()
{
    if (!m_lspActive || !m_lspManager || m_fileName.isEmpty())
        return;

    QString uri = lsp::uriFromPath(m_fileName);
    auto* client = m_lspManager->clientForUri(uri);
    if (!client || !client->capabilities().typeDefinitionProvider)
        return;

    int line, col;
    getCursorPosition(&line, &col);
    lsp::Position pos{line, col};

    client->requestTypeDefinition(uri, pos);
}

void CodeEditor::goToDeclaration()
{
    if (!m_lspActive || !m_lspManager || m_fileName.isEmpty())
        return;

    QString uri = lsp::uriFromPath(m_fileName);
    auto* client = m_lspManager->clientForUri(uri);
    if (!client || !client->capabilities().declarationProvider)
        return;

    int line, col;
    getCursorPosition(&line, &col);
    lsp::Position pos{line, col};

    client->requestDeclaration(uri, pos);
}

void CodeEditor::findReferences()
{
    if (!m_lspActive || !m_lspManager || m_fileName.isEmpty())
        return;

    QString uri = lsp::uriFromPath(m_fileName);
    auto* client = m_lspManager->clientForUri(uri);
    if (!client || !client->capabilities().referencesProvider)
        return;

    int line, col;
    getCursorPosition(&line, &col);
    lsp::Position pos{line, col};

    client->requestReferences(uri, pos);
}

void CodeEditor::formatSelection()
{
    if (!m_lspActive || !m_lspManager || m_fileName.isEmpty())
        return;

    if (!hasSelectedText())
        return;

    QString uri = lsp::uriFromPath(m_fileName);
    auto* client = m_lspManager->clientForUri(uri);
    if (!client || !client->capabilities().formattingProvider)
        return;

    int lineFrom, colFrom, lineTo, colTo;
    getSelection(&lineFrom, &colFrom, &lineTo, &colTo);

    lsp::Range range{{lineFrom, colFrom}, {lineTo, colTo}};
    int tabSize = tabWidth();
    bool insertSpaces = !indentationsUseTabs();

    client->requestRangeFormatting(uri, range, tabSize, insertSpaces);
}

void CodeEditor::requestCodeActions()
{
    if (!m_lspActive || !m_lspManager || m_fileName.isEmpty())
        return;

    QString uri = lsp::uriFromPath(m_fileName);
    auto* client = m_lspManager->clientForUri(uri);
    if (!client || !client->capabilities().codeActionProvider)
        return;

    int line, col;
    getCursorPosition(&line, &col);
    lsp::Range range{{line, 0}, {line + 1, 0}};
    client->requestCodeAction(uri, range, {});
}

void CodeEditor::expandSelection()
{
    if (!m_lspActive || !m_lspManager || m_fileName.isEmpty())
        return;

    QString uri = lsp::uriFromPath(m_fileName);
    auto* client = m_lspManager->clientForUri(uri);
    if (!client || !client->capabilities().selectionRangeProvider)
        return;

    // If we already have a stack from a previous query, go outward
    if (!m_selectionRangeStack.isEmpty() && m_selectionRangeDepth > 0) {
        m_selectionRangeDepth--;
        const auto& r = m_selectionRangeStack[m_selectionRangeDepth];
        setSelection(r.start.line, r.start.character, r.end.line, r.end.character);
        return;
    }

    int line, col;
    getCursorPosition(&line, &col);
    QList<lsp::Position> positions{{line, col}};
    client->requestSelectionRanges(uri, positions);
}

static void flattenSelectionRanges(const QJsonObject& node, QList<lsp::Range>& out)
{
    if (node.contains("range")) {
        out.append(lsp::Range::fromJson(node["range"].toObject()));
    }
    QJsonArray children = node["children"].toArray();
    for (const auto& child : children) {
        flattenSelectionRanges(child.toObject(), out);
    }
}

void CodeEditor::setSelectionRanges(const QJsonArray& ranges)
{
    m_selectionRangeStack.clear();
    if (ranges.isEmpty())
        return;

    QJsonObject root = ranges.first().toObject();
    flattenSelectionRanges(root, m_selectionRangeStack);

    if (m_selectionRangeStack.isEmpty())
        return;

    m_selectionRangeDepth = 0;
    const auto& r = m_selectionRangeStack.first();
    setSelection(r.start.line, r.start.character, r.end.line, r.end.character);
}

void CodeEditor::shrinkSelection()
{
    if (m_selectionRangeStack.isEmpty() || m_selectionRangeDepth >= m_selectionRangeStack.size() - 1)
        return;

    m_selectionRangeDepth++;
    const auto& r = m_selectionRangeStack[m_selectionRangeDepth];
    setSelection(r.start.line, r.start.character, r.end.line, r.end.character);
}

void CodeEditor::requestSelectionRanges()
{
    expandSelection();
}

void CodeEditor::requestRename()
{
    if (!m_lspActive || !m_lspManager || m_fileName.isEmpty())
        return;

    QString uri = lsp::uriFromPath(m_fileName);
    auto* client = m_lspManager->clientForUri(uri);
    if (!client || !client->capabilities().renameProvider)
        return;

    int line, col;
    getCursorPosition(&line, &col);
    lsp::Position pos{line, col};

    bool ok = false;
    QString newName = QInputDialog::getText(nullptr, tr("Rename Symbol"), tr("New name:"),
                                            QLineEdit::Normal, QString(), &ok);
    if (!ok || newName.isEmpty())
        return;

    client->requestRename(uri, pos, newName);
}

void CodeEditor::requestDocumentHighlight()
{
    if (!m_lspActive || !m_lspManager || m_fileName.isEmpty())
        return;

    QString uri = lsp::uriFromPath(m_fileName);
    auto* client = m_lspManager->clientForUri(uri);
    if (!client || !client->capabilities().documentHighlightProvider)
        return;

    int line, col;
    getCursorPosition(&line, &col);
    lsp::Position pos{line, col};

    client->requestDocumentHighlight(uri, pos);
}

void CodeEditor::showCompletion(const lsp::CompletionList& completions)
{
    m_lspCompletionItems = completions.items;
    m_lspCompletionActive = true;

    if (completions.items.isEmpty())
        return;

    // Build the auto-completion list
    QStringList entries;
    for (const auto& item : completions.items) {
        QString label = item.label;
        if (!item.detail.isEmpty())
            label += QStringLiteral(" ?") + item.detail;
        entries.append(label);
    }

    if (entries.isEmpty())
        return;

    // Get current word for sorting/filtering
    int pos = SendScintilla(SCI_GETCURRENTPOS);
    int start = pos;
    while (start > 0) {
        char c = static_cast<char>(SendScintilla(SCI_GETCHARAT, start - 1));
        if (std::isalnum(c) || c == '_')
            start--;
        else
            break;
    }
    QString currentWord = text(start, pos);

    // Show autocompletion list using SCI_AUTOCSHOW
    constexpr char sep = '\n';
    QString joined = entries.join(QLatin1Char(sep));
    QByteArray data = joined.toUtf8();
    SendScintilla(SCI_AUTOCSHOW, static_cast<uintptr_t>(currentWord.length()), data.constData());
}

void CodeEditor::applyDiagnostics(const QString& uri, const QList<lsp::Diagnostic>& diagnostics)
{
    Q_UNUSED(uri)

    // Clear existing diagnostics
    clearDiagnostics();

    // Apply indicators and markers for each diagnostic
    for (const auto& d : diagnostics) {
        if (d.range.start.line < 0 || d.range.end.line < 0)
            continue;

        int indicatorId = lsp::LSP_INDICATOR_ERROR;

        switch (d.severityLevel) {
        case 1:
            indicatorId = lsp::LSP_INDICATOR_ERROR;
            break;
        case 2:
            indicatorId = lsp::LSP_INDICATOR_WARNING;
            break;
        case 3:
        case 4:
            indicatorId = lsp::LSP_INDICATOR_INFO;
            break;
        }

        fillIndicatorRange(d.range.start.line, d.range.start.character,
                           d.range.end.line, d.range.end.character, indicatorId);

    }

    emit diagnosticsChanged(uri, diagnostics);
}

void CodeEditor::clearDiagnostics()
{
    if (lines() == 0)
        return;
    clearIndicatorRange(0, 0, lines() - 1, 0, lsp::LSP_INDICATOR_ERROR);
    clearIndicatorRange(0, 0, lines() - 1, 0, lsp::LSP_INDICATOR_WARNING);
    clearIndicatorRange(0, 0, lines() - 1, 0, lsp::LSP_INDICATOR_INFO);
}

void CodeEditor::applyHighlights(const QJsonArray& highlights)
{
    clearHighlights();

    for (const auto& h : highlights) {
        QJsonObject obj = h.toObject();
        lsp::Range range = lsp::Range::fromJson(obj["range"].toObject());

        if (range.start.line < 0 || range.end.line < 0)
            continue;

        fillIndicatorRange(range.start.line, range.start.character,
                           range.end.line, range.end.character,
                           lsp::LSP_INDICATOR_HIGHLIGHT);
    }
}

void CodeEditor::clearHighlights()
{
    clearIndicatorRange(0, 0, lines() - 1, 0, lsp::LSP_INDICATOR_HIGHLIGHT);
}

void CodeEditor::setLinkedEditingRanges(const QJsonObject& result)
{
    m_linkedRanges.clear();

    QJsonArray ranges = result["ranges"].toArray();
    for (const auto& r : ranges) {
        m_linkedRanges.append(lsp::Range::fromJson(r.toObject()));
    }
}

void CodeEditor::clearLinkedRanges()
{
    m_linkedRanges.clear();
    m_isApplyingLinkedEdit = false;
}

void CodeEditor::applySemanticTokens(const QString& uri, const QJsonArray& tokenData)
{
    if (uri != lsp::uriFromPath(m_fileName)) return;
    if (tokenData.isEmpty()) return;

    m_semanticTokensUri = uri;

    // Decode the flat token array: [line, col, length, type, modifier, ...]
    // Apply using indicator 27 (semantic tokens)
    int line = 0, col = 0;
    for (int i = 0; i + 4 < tokenData.size(); i += 5) {
        int dLine = tokenData[i].toInt();
        int dCol = tokenData[i + 1].toInt();
        int length = tokenData[i + 2].toInt();
        // int type = tokenData[i + 3].toInt(); // Could use for color selection
        // int mod = tokenData[i + 4].toInt();

        if (dLine == 0) {
            col += dCol;
        } else {
            line += dLine;
            col = dCol;
        }

        if (length > 0 && line >= 0 && line < lines()) {
            for (int c = 0; c < length; ++c)
                fillIndicatorRange(line, col + c, line, col + c + 1, lsp::LSP_INDICATOR_SEMANTIC);
        }
    }
}

bool CodeEditor::hasLineMarker(int line, int marker) const
{
    // Check if a specific line has a specific marker
    unsigned int mask = markersAtLine(line);
    return (mask & (1 << marker)) != 0;
}

void CodeEditor::replaceSelectedText(const QString &text)
{
    if (isReadOnly())
        return;
    removeSelectedText();
    insert(text);
}

int CodeEditor::lineFromPosition(int pos) const
{
    return static_cast<int>(SendScintilla(SCI_LINEFROMPOSITION, pos));
}

int CodeEditor::cursorPosition() const
{
    return static_cast<int>(SendScintilla(SCI_GETCURRENTPOS));
}

void CodeEditor::showToolTip(int pos, const QString& text)
{
    int x = static_cast<int>(SendScintilla(SCI_POINTXFROMPOSITION, 0, pos));
    int y = static_cast<int>(SendScintilla(SCI_POINTYFROMPOSITION, 0, pos));
    QToolTip::showText(mapToGlobal(QPoint(x, y)), text, this);
}

void CodeEditor::applyFormattingEdits(const QList<QJsonObject>& edits)
{
    beginUndoAction();
    for (const auto& edit : edits) {
        auto range = lsp::Range::fromJson(edit["range"].toObject());
        QString newText = edit["newText"].toString();

        int startPos = static_cast<int>(SendScintilla(SCI_COLUMNTOBYPOS, range.start.line, range.start.character));
        int endPos = static_cast<int>(SendScintilla(SCI_COLUMNTOBYPOS, range.end.line, range.end.character));

        SendScintilla(SCI_SETSEL, startPos, endPos);
        SendScintilla(SCI_REPLACESEL, static_cast<uintptr_t>(0), newText.toUtf8().constData());
    }
    endUndoAction();
}

void CodeEditor::showSignatureHelp(const QJsonObject& info)
{
    QJsonArray signatures = info["signatures"].toArray();
    if (signatures.isEmpty()) return;

    QStringList parts;
    for (const auto& sig : signatures) {
        QJsonObject s = sig.toObject();
        QString label = s["label"].toString();
        parts.append(label);
    }

    int line, col;
    getCursorPosition(&line, &col);
    int pos = static_cast<int>(SendScintilla(SCI_GETCURRENTPOS));
    int x = static_cast<int>(SendScintilla(SCI_POINTXFROMPOSITION, 0, pos));
    int y = static_cast<int>(SendScintilla(SCI_POINTYFROMPOSITION, 0, pos));

    QToolTip::showText(mapToGlobal(QPoint(x, y)), parts.join("\n"), this);
}


