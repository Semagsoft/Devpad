/*
 * Devpad - A C++/Qt6 code editor
 * Copyright (C) 2026 Semagsoft
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */
#include "codeeditor.h"

#include "languageinfo.h"
#include "settingsmanager.h"
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

constexpr int CARETSTYLE_INVISIBLE = 0;
constexpr int CARETSTYLE_LINE = 1;
constexpr int CARETSTYLE_BLOCK = 2;
constexpr int CARETSTYLE_UNDERLINE = 4;

constexpr int EDGE_NONE = 0;
constexpr int EDGE_LINE = 1;
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

    // Enable code folding
    setFolding(QsciScintilla::BoxedTreeFoldStyle);
    setFoldMarginColors(QColor(220, 220, 220), QColor(200, 200, 200));

    // Brace matching
    setBraceMatching(QsciScintilla::SloppyBraceMatch);
    setMatchedBraceBackgroundColor(QColor(255, 255, 200));
    setMatchedBraceForegroundColor(Qt::darkGreen);

    // Auto indentation
    setAutoIndent(true);
    setTabWidth(4);
    setIndentationsUseTabs(false);

    // Current line highlighting
    setCaretLineVisible(true);
    setCaretLineBackgroundColor(QColor(232, 242, 254));

    // Bookmark margin
    setMarginType(BOOKMARK_MARGIN, QsciScintilla::SymbolMargin);
    setMarginWidth(BOOKMARK_MARGIN, 16);
    setMarginSensitivity(BOOKMARK_MARGIN, true);
    setMarginMarkerMask(BOOKMARK_MARGIN, (1 << MARKER_BOOKMARK));
    markerDefine(QsciScintilla::SC_MARK_BOOKMARK, MARKER_BOOKMARK);
    setMarkerForegroundColor(QColor(255, 160, 0), MARKER_BOOKMARK);
    setMarkerBackgroundColor(QColor(255, 200, 100), MARKER_BOOKMARK);
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

    setLexer(lexer);
    applyLexerTheme();
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
    // QScintilla's setModified(true) is documented as a no-op.
    // Work around by inserting and deleting a character, advancing
    // past the save point in the undo stack without changing content.
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
        return;
    }

    setAutoCompletionSource(QsciScintilla::AcsAll);
    setAutoCompletionThreshold(threshold);
    setAutoCompletionCaseSensitivity(caseSensitive);
    setAutoCompletionReplaceWord(true);
    setAutoCompletionShowSingle(false);
    setAutoCompletionFillupsEnabled(true);
    setAutoCompletionUseSingle(QsciScintilla::AcusExplicit);
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
