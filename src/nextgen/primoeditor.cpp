/*
 * Devpad - A C++/Qt6 code editor
 * Copyright (C) 2026 Semagsoft
 */

#include "primoeditor.h"

#include "core/fileservice.h"
#include "managers/settingsmanager.h"
#include "primoshader.h"
#include "keywords.h"
#include "theme.h"

#include <QClipboard>
#include <QFileInfo>
#include <QFontMetricsF>
#include <QGuiApplication>
#include <QKeyEvent>
#include <QMouseEvent>
#include <QQuickWindow>
#include <QSGFlatColorMaterial>
#include <QSGGeometry>
#include <QSGGeometryNode>
#include <QSGRectangleNode>
#include <QSGSimpleRectNode>
#include <QSGTextNode>
#include <QTextLayout>

namespace
{
// Maximum number of lines to cache in m_linesCache / m_highlightCache.
// For larger documents, lines are computed on demand to bound memory usage.
constexpr int MaxCachedLines = 20000;
} // namespace

PrimoEditor::PrimoEditor(QQuickItem* parent) : QQuickItem(parent)
{
    setFlag(ItemHasContents, true);
    setAcceptedMouseButtons(Qt::LeftButton);
    setAcceptHoverEvents(true);
    setFocus(true);
    setActiveFocusOnTab(true);

    m_doc = new PrimoDocument(this);
    connect(m_doc, &PrimoDocument::textChanged, this,
            [this]()
            {
                m_linesDirty = true;
                m_highlightDirty = true;
                m_highlightVersion++;
                emit textChanged();
                update();
                scheduleHighlight();
            });
    connect(m_doc, &PrimoDocument::filePathChanged, this, &PrimoEditor::filePathChanged);
    connect(m_doc, &PrimoDocument::languageChanged, this,
            [this]()
            {
                emit languageChanged();
                m_highlightDirty = true;
                m_highlightVersion++;
                scheduleHighlight();
                update();
            });
    connect(m_doc, &PrimoDocument::canUndoChanged, this, &PrimoEditor::canUndoChanged);
    connect(m_doc, &PrimoDocument::canRedoChanged, this, &PrimoEditor::canRedoChanged);
    connect(m_doc, &PrimoDocument::undoDisabledChanged, this, &PrimoEditor::undoDisabledChanged);

    m_font = SettingsManager::instance().defaultFont();
    m_tabWidth = SettingsManager::instance().tabWidth();
    m_wordWrap = SettingsManager::instance().wordWrap();
    // Theme from settings
    ThemeId tid = SettingsManager::instance().theme();
    m_theme = getThemeColors(tid);
    m_bg = m_theme.background;
    m_fg = m_theme.foreground;
    ensureMetrics();

    m_highlightController = new PrimoHighlighterController(this);
    connect(m_highlightController, &PrimoHighlighterController::highlighted, this,
            [this](int version, int first, QVector<QVector<QTextLayout::FormatRange>> fmts) { applyHighlightResult(version, first, fmts); });

    connect(this, &PrimoEditor::textChanged, this,
            [this]()
            {
                qreal nh = contentHeight();
                if (!qFuzzyCompare(nh, m_contentHeight))
                {
                    m_contentHeight = nh;
                    emit contentHeightChanged();
                    update();
                }
            });
    connect(this, &PrimoEditor::fontChanged, this,
            [this]()
            {
                ensureMetrics();
                m_contentHeight = contentHeight();
                emit contentHeightChanged();
                update();
            });

    m_contentHeight = contentHeight();

    // Listen for global settings changes (theme, font, etc.) so the nextgen
    // editor updates live instead of only at construction time.
    connect(&SettingsManager::instance(), &SettingsManager::settingsChanged, this,
            &PrimoEditor::applySettings);
}

void PrimoEditor::applySettings()
{
    // Theme
    ThemeId tid = SettingsManager::instance().theme();
    m_theme = getThemeColors(tid);
    m_bg = m_theme.background;
    m_fg = m_theme.foreground;
    emit backgroundColorChanged();
    emit foregroundColorChanged();
    update();

    // Font
    m_font = SettingsManager::instance().defaultFont();
    m_tabWidth = SettingsManager::instance().tabWidth();
    m_wordWrap = SettingsManager::instance().wordWrap();
    ensureMetrics();
    emit fontChanged();
    emit tabWidthChanged();
    emit wordWrapChanged();

    // Re-highlight with new theme
    m_highlightDirty = true;
    m_highlightVersion++;
    scheduleHighlight();
    update();
}

QString PrimoEditor::filePath() const
{
    return m_doc ? m_doc->filePath() : QString();
}
void PrimoEditor::setFilePath(const QString& path)
{
    if (m_doc)
        m_doc->setFilePath(path);
}
QString PrimoEditor::text() const
{
    return m_doc ? m_doc->text() : QString();
}
void PrimoEditor::setText(const QString& t)
{
    if (m_doc)
        m_doc->setText(t);
}
QString PrimoEditor::language() const
{
    return m_doc ? m_doc->language() : QString();
}
void PrimoEditor::setLanguage(const QString& lang)
{
    if (m_doc)
        m_doc->setLanguage(lang);
}
QString PrimoEditor::encoding() const
{
    return m_encoding;
}
void PrimoEditor::setEncoding(const QString& enc)
{
    if (m_encoding != enc)
    {
        m_encoding = enc;
        emit encodingChanged();
    }
}
QFont PrimoEditor::font() const
{
    return m_font;
}
void PrimoEditor::setFont(const QFont& f)
{
    if (m_font == f)
        return;
    m_font = f;
    ensureMetrics();
    emit fontChanged();
    update();
}
int PrimoEditor::tabWidth() const
{
    return m_tabWidth;
}
void PrimoEditor::setTabWidth(int w)
{
    if (m_tabWidth != w)
    {
        m_tabWidth = w;
        emit tabWidthChanged();
        update();
    }
}
bool PrimoEditor::wordWrap() const
{
    return m_wordWrap;
}
void PrimoEditor::setWordWrap(bool w)
{
    if (m_wordWrap != w)
    {
        m_wordWrap = w;
        emit wordWrapChanged();
        m_wrapScrollTop = 0;
        emit wrapScrollTopChanged();
        update();
    }
}

qreal PrimoEditor::hScroll() const
{
    return m_hScroll;
}
void PrimoEditor::setHScroll(qreal h)
{
    if (qFuzzyCompare(m_hScroll, h))
        return;
    m_hScroll = qMax(0.0, h);
    emit hScrollChanged();
    update();
}
qreal PrimoEditor::wrapScrollTop() const
{
    return m_wrapScrollTop;
}
void PrimoEditor::setWrapScrollTop(qreal v)
{
    if (qFuzzyCompare(m_wrapScrollTop, v))
        return;
    m_wrapScrollTop = qMax(0.0, v);
    emit wrapScrollTopChanged();
    update();
}
int PrimoEditor::cursorLine() const
{
    return m_cursorLine;
}
void PrimoEditor::setCursorLine(int line)
{
    if (m_cursorLine != line)
    {
        m_cursorLine = line;
        emit cursorChanged();
        update();
    }
}
int PrimoEditor::cursorColumn() const
{
    return m_cursorColumn;
}
void PrimoEditor::setCursorColumn(int col)
{
    if (m_cursorColumn != col)
    {
        m_cursorColumn = col;
        emit cursorChanged();
        update();
    }
}
qreal PrimoEditor::contentHeight() const
{
    if (m_wordWrap && m_doc)
    {
        // Word wrap: total height = total wrapped visual rows * lineHeight
        QString txt = m_doc->text();
        if (txt.isEmpty())
            return m_lineHeight + 8;
        QFontMetricsF fm(m_font);
        QQuickWindow* win = window();
        qreal winWidth = win ? win->width() : boundingRect().width();
        qreal gutterW = m_gutterVisible ? m_gutterWidth : 0;
        int availGlobal = qMax(1, int(winWidth - gutterW - 16));
        int totalWrapRows = 0;
        int total = m_doc->lineCount();
        for (int i = 0; i < total; ++i)
        {
            QString line = m_doc->lineAt(i);
            if (line.isEmpty())
            {
                ++totalWrapRows;
            }
            else
            {
                QString disp = expandedTab(line);
                int len = disp.size();
                totalWrapRows += (len + availGlobal - 1) / qMax(1, availGlobal);
            }
        }
        return totalWrapRows * m_lineHeight + 8;
    }
    if (m_linesDirty)
    {
        QString txt = m_doc ? m_doc->text() : QString();
        int lines = txt.isEmpty() ? 1 : txt.count(QLatin1Char('\n')) + 1;
        return lines * m_lineHeight + 8;
    }
    int lines;
    if (!m_linesCache.isEmpty())
    {
        lines = m_linesCache.size();
    }
    else if (m_doc && !m_doc->text().isEmpty())
    {
        lines = m_doc->text().count(QLatin1Char('\n')) + 1;
    }
    else
    {
        lines = 1;
    }
    if (m_linesCache.isEmpty() && m_doc)
    {
        lines = m_doc->text().isEmpty() ? 1 : m_doc->text().count(QLatin1Char('\n')) + 1;
    }
    return lines * m_lineHeight + 8;
}
qreal PrimoEditor::lineHeight() const
{
    return m_lineHeight;
}
QColor PrimoEditor::backgroundColor() const
{
    return m_bg;
}
void PrimoEditor::setBackgroundColor(const QColor& c)
{
    if (m_bg != c)
    {
        m_bg = c;
        emit backgroundColorChanged();
        update();
    }
}
QColor PrimoEditor::foregroundColor() const
{
    return m_fg;
}
void PrimoEditor::setForegroundColor(const QColor& c)
{
    if (m_fg != c)
    {
        m_fg = c;
        emit foregroundColorChanged();
        update();
    }
}
bool PrimoEditor::gutterVisible() const
{
    return m_gutterVisible;
}
void PrimoEditor::setGutterVisible(bool v)
{
    if (m_gutterVisible != v)
    {
        m_gutterVisible = v;
        emit gutterVisibleChanged();
        ensureMetrics();
        update();
    }
}
bool PrimoEditor::showLineNumbers() const
{
    return m_showLineNumbers;
}
void PrimoEditor::setShowLineNumbers(bool v)
{
    if (m_showLineNumbers != v)
    {
        m_showLineNumbers = v;
        emit showLineNumbersChanged();
        update();
    }
}
bool PrimoEditor::relativeNumbers() const
{
    return m_relativeNumbers;
}
void PrimoEditor::setRelativeNumbers(bool v)
{
    if (m_relativeNumbers != v)
    {
        m_relativeNumbers = v;
        emit relativeNumbersChanged();
        update();
    }
}
bool PrimoEditor::isReadOnly() const
{
    return m_readOnly;
}
void PrimoEditor::setReadOnly(bool ro)
{
    if (m_readOnly != ro)
    {
        m_readOnly = ro;
        emit readOnlyChanged();
        update();
    }
}
bool PrimoEditor::minimapVisible() const
{
    return m_minimapVisible;
}
void PrimoEditor::setMinimapVisible(bool v)
{
    if (m_minimapVisible != v)
    {
        m_minimapVisible = v;
        emit minimapVisibleChanged();
        update();
    }
}
qreal PrimoEditor::gutterWidth() const
{
    return m_gutterWidth;
}
PrimoDocument* PrimoEditor::document() const
{
    return m_doc;
}

void PrimoEditor::toggleBookmark(int line)
{
    if (line < 0)
        return;
    if (m_bookmarks.contains(line))
        m_bookmarks.remove(line);
    else
        m_bookmarks.insert(line);
    emit bookmarksChanged();
    update();
}
void PrimoEditor::toggleBookmarkCurrent()
{
    toggleBookmark(m_cursorLine);
}
bool PrimoEditor::hasBookmark(int line) const
{
    return m_bookmarks.contains(line);
}
QList<int> PrimoEditor::bookmarkLines() const
{
    QList<int> l = m_bookmarks.values();
    std::sort(l.begin(), l.end());
    return l;
}
void PrimoEditor::clearBookmarks()
{
    m_bookmarks.clear();
    emit bookmarksChanged();
    update();
}
void PrimoEditor::nextBookmark()
{
    QList<int> lines = m_bookmarks.values();
    std::sort(lines.begin(), lines.end());
    for (int l : lines)
    {
        if (l > m_cursorLine)
        {
            m_cursorLine = l;
            m_cursorColumn = 0;
            emit cursorChanged();
            update();
            return;
        }
    }
    // wrap to first
    if (!lines.isEmpty())
    {
        m_cursorLine = lines.first();
        m_cursorColumn = 0;
        emit cursorChanged();
        update();
    }
}
void PrimoEditor::prevBookmark()
{
    QList<int> lines = m_bookmarks.values();
    std::sort(lines.begin(), lines.end(), std::greater<int>());
    for (int l : lines)
    {
        if (l < m_cursorLine)
        {
            m_cursorLine = l;
            m_cursorColumn = 0;
            emit cursorChanged();
            update();
            return;
        }
    }
    // wrap to last
    if (!lines.isEmpty())
    {
        m_cursorLine = lines.last();
        m_cursorColumn = 0;
        emit cursorChanged();
        update();
    }
}

static QString escapeRegex(const QString& s)
{
    QString out;
    out.reserve(s.size());
    for (QChar ch : s)
    {
        if (ch == QLatin1Char('\\') || ch == QLatin1Char('.') || ch == QLatin1Char('+') ||
            ch == QLatin1Char('*') || ch == QLatin1Char('?') || ch == QLatin1Char('^') ||
            ch == QLatin1Char('$') || ch == QLatin1Char('{') || ch == QLatin1Char('}') ||
            ch == QLatin1Char('[') || ch == QLatin1Char(']') || ch == QLatin1Char('(') ||
            ch == QLatin1Char(')') || ch == QLatin1Char('|'))
        {
            out += QLatin1Char('\\');
        }
        out += ch;
    }
    return out;
}

void PrimoEditor::findNext(const QString& pattern, bool caseSensitive, bool wholeWord, bool regex)
{
    if (pattern.isEmpty() || !m_doc)
        return;
    QString txt = m_doc->text();
    QRegularExpression re;
    if (regex)
        re = QRegularExpression(pattern, caseSensitive ? QRegularExpression::NoPatternOption : QRegularExpression::CaseInsensitiveOption);
    else
    {
        QString p = escapeRegex(pattern);
        if (wholeWord)
            p = QStringLiteral("\\b") + p + QStringLiteral("\\b");
        re = QRegularExpression(p, caseSensitive ? QRegularExpression::NoPatternOption : QRegularExpression::CaseInsensitiveOption);
    }
    if (!re.isValid())
        return;
    // Compute current offset
    QStringList lines = txt.split(QLatin1Char('\n'));
    int curOffset = 0;
    for (int i = 0; i < m_cursorLine && i < lines.size(); ++i)
        curOffset += lines.at(i).size() + 1;
    curOffset += qMin(m_cursorColumn, lines.value(m_cursorLine).size());
    int matchPos = -1;
    QRegularExpressionMatchIterator it = re.globalMatch(txt);
    QVector<int> positions;
    while (it.hasNext())
    {
        QRegularExpressionMatch m = it.next();
        positions.append(m.capturedStart());
    }
    for (int pos : positions)
    {
        if (pos > curOffset)
        {
            matchPos = pos;
            break;
        }
    }
    if (matchPos == -1 && !positions.isEmpty())
        matchPos = positions.first();
    if (matchPos == -1)
        return;
    // Convert offset to line/col
    int off = 0, line = 0, col = 0;
    for (int l = 0; l < lines.size(); ++l)
    {
        int len = lines.at(l).size() + 1;
        if (matchPos < off + len)
        {
            line = l;
            col = matchPos - off;
            break;
        }
        off += len;
    }
    m_cursorLine = line;
    m_cursorColumn = col;
    emit cursorChanged();
    update();
}

void PrimoEditor::findPrevious(const QString& pattern, bool caseSensitive, bool wholeWord, bool regex)
{
    if (pattern.isEmpty() || !m_doc)
        return;
    QString txt = m_doc->text();
    QRegularExpression re;
    if (regex)
        re = QRegularExpression(pattern, caseSensitive ? QRegularExpression::NoPatternOption : QRegularExpression::CaseInsensitiveOption);
    else
    {
        QString p = escapeRegex(pattern);
        if (wholeWord)
            p = QStringLiteral("\\b") + p + QStringLiteral("\\b");
        re = QRegularExpression(p, caseSensitive ? QRegularExpression::NoPatternOption : QRegularExpression::CaseInsensitiveOption);
    }
    if (!re.isValid())
        return;
    QStringList lines = txt.split(QLatin1Char('\n'));
    int curOffset = 0;
    for (int i = 0; i < m_cursorLine && i < lines.size(); ++i)
        curOffset += lines.at(i).size() + 1;
    curOffset += qMin(m_cursorColumn, lines.value(m_cursorLine).size());
    int matchPos = -1;
    QRegularExpressionMatchIterator it = re.globalMatch(txt);
    QVector<int> positions;
    while (it.hasNext())
    {
        QRegularExpressionMatch m = it.next();
        positions.append(m.capturedStart());
    }
    for (int i = positions.size() - 1; i >= 0; --i)
    {
        if (positions.at(i) < curOffset)
        {
            matchPos = positions.at(i);
            break;
        }
    }
    if (matchPos == -1 && !positions.isEmpty())
        matchPos = positions.last();
    if (matchPos == -1)
        return;
    int off = 0, line = 0, col = 0;
    for (int l = 0; l < lines.size(); ++l)
    {
        int len = lines.at(l).size() + 1;
        if (matchPos < off + len)
        {
            line = l;
            col = matchPos - off;
            break;
        }
        off += len;
    }
    m_cursorLine = line;
    m_cursorColumn = col;
    emit cursorChanged();
    update();
}

bool PrimoEditor::hasSelection() const
{
    return m_selAnchorLine != -1 && (m_selAnchorLine != m_cursorLine || m_selAnchorCol != m_cursorColumn);
}

QString PrimoEditor::selectedText() const
{
    if (!hasSelection() || !m_doc)
        return QString();
    int aLine = m_selAnchorLine;
    int aCol = m_selAnchorCol;
    int cLine = m_cursorLine;
    int cCol = m_cursorColumn;
    if (aLine > cLine || (aLine == cLine && aCol > cCol))
    {
        std::swap(aLine, cLine);
        std::swap(aCol, cCol);
    }
    QString txt = m_doc->text();
    QStringList lines = txt.split(QLatin1Char('\n'));
    if (aLine == cLine)
    {
        return lines.value(aLine).mid(aCol, cCol - aCol);
    }
    QStringList parts;
    parts.append(lines.value(aLine).mid(aCol));
    for (int l = aLine + 1; l < cLine; ++l)
        parts.append(lines.value(l));
    parts.append(lines.value(cLine).left(cCol));
    return parts.join(QLatin1Char('\n'));
}

void PrimoEditor::setSelectionAnchor(int line, int col)
{
    m_selAnchorLine = line;
    m_selAnchorCol = col;
    update();
}

void PrimoEditor::clearSelection()
{
    m_selAnchorLine = -1;
    m_selAnchorCol = -1;
    update();
}

void PrimoEditor::deleteSelection()
{
    if (!hasSelection() || !m_doc)
        return;
    int aLine = m_selAnchorLine;
    int aCol = m_selAnchorCol;
    int cLine = m_cursorLine;
    int cCol = m_cursorColumn;
    if (aLine > cLine || (aLine == cLine && aCol > cCol))
    {
        std::swap(aLine, cLine);
        std::swap(aCol, cCol);
    }
    QString txt = m_doc->text();
    QStringList lines = txt.split(QLatin1Char('\n'));
    if (aLine == cLine)
    {
        lines[aLine].remove(aCol, cCol - aCol);
        m_cursorColumn = aCol;
    }
    else
    {
        QString suffix = lines[cLine].left(cCol);
        lines[aLine] = lines[aLine].left(aCol) + suffix;
        for (int l = cLine; l > aLine; --l)
            lines.removeAt(l);
        m_cursorLine = aLine;
        m_cursorColumn = aCol;
    }
    m_doc->setText(lines.join(QLatin1Char('\n')));
    clearSelection();
    m_cursorLine = qMin(m_cursorLine, lines.size() - 1);
    m_cursorColumn = qMin(m_cursorColumn, lines.value(m_cursorLine).size());
    emit cursorChanged();
    update();
}

int PrimoEditor::selectionAnchorLine() const
{
    return m_selAnchorLine;
}

int PrimoEditor::selectionAnchorCol() const
{
    return m_selAnchorCol;
}

void PrimoEditor::setDiagnostics(const QList<int>& lines)
{
    m_diagLines = QSet<int>(lines.begin(), lines.end());
    emit diagnosticsChanged();
    update();
}
void PrimoEditor::clearDiagnostics()
{
    m_diagLines.clear();
    emit diagnosticsChanged();
    update();
}
void PrimoEditor::setSearchMatches(const QList<int>& positions)
{
    // positions is a flat list: [start1, len1, start2, len2, ...]
    m_searchMatches.clear();
    for (int i = 0; i + 1 < positions.size(); i += 2)
        m_searchMatches.append(qMakePair(positions[i], positions[i + 1]));
    update();
}
void PrimoEditor::clearSearchMatches()
{
    m_searchMatches.clear();
    update();
}

QStringList PrimoEditor::completionList(const QString& prefix) const
{
    QStringList result;
    if (!m_doc)
        return result;
    QString lang = m_doc->language().isEmpty() ? QStringLiteral("cpp") : m_doc->language();
    const QStringList* keywords = nullptr;
    if (lang == QStringLiteral("cpp")) keywords = &cppKeywords();
    else if (lang == QStringLiteral("csharp")) keywords = &csharpKeywords();
    else if (lang == QStringLiteral("java")) keywords = &javaKeywords();
    else if (lang == QStringLiteral("python")) keywords = &pythonKeywords();
    else if (lang == QStringLiteral("javascript")) keywords = &javascriptKeywords();
    else if (lang == QStringLiteral("typescript")) keywords = &typescriptKeywords();
    else if (lang == QStringLiteral("html")) keywords = &htmlKeywords();
    else if (lang == QStringLiteral("css")) keywords = &cssKeywords();
    else if (lang == QStringLiteral("xml")) keywords = &xmlKeywords();
    else if (lang == QStringLiteral("sql")) keywords = &sqlKeywords();
    else if (lang == QStringLiteral("bash")) keywords = &bashKeywords();
    else if (lang == QStringLiteral("lua")) keywords = &luaKeywords();
    else if (lang == QStringLiteral("go")) keywords = &goKeywords();
    else if (lang == QStringLiteral("rust")) keywords = &rustKeywords();
    else if (lang == QStringLiteral("php")) keywords = &phpKeywords();
    else if (lang == QStringLiteral("markdown")) keywords = &markdownKeywords();
    else if (lang == QStringLiteral("json")) keywords = &jsonKeywords();
    else if (lang == QStringLiteral("qml")) keywords = &qmlKeywords();
    else if (lang == QStringLiteral("cmake")) keywords = &cmakeKeywords();
    else keywords = &cppKeywords();
    if (keywords)
    {
        for (const QString& kw : *keywords)
        {
            if (kw.startsWith(prefix, Qt::CaseInsensitive))
                result << kw;
        }
    }
    return result;
}

void PrimoEditor::insertSnippet(const QString& trigger)
{
    if (m_readOnly || !m_doc)
        return;
    QString snippet;
    if (trigger == QStringLiteral("for"))
        snippet = QStringLiteral("for (int i = 0; i < $1; ++i) {\n\t$2\n}");
    else if (trigger == QStringLiteral("if"))
        snippet = QStringLiteral("if ($1) {\n\t$2\n}");
    else if (trigger == QStringLiteral("else"))
        snippet = QStringLiteral("else {\n\t$1\n}");
    else if (trigger == QStringLiteral("func") || trigger == QStringLiteral("fn"))
        snippet = QStringLiteral("$1($2) {\n\t$3\n}");
    else if (trigger == QStringLiteral("class"))
        snippet = QStringLiteral("class $1 {\npublic:\n\t$2\n};\n");
    else if (trigger == QStringLiteral("main"))
        snippet = QStringLiteral("int main(int argc, char* argv[]) {\n\treturn 0;\n}");
    else
        return;
    // Replace the trigger word with the snippet
    QString txt = m_doc->text();
    QStringList lines = txt.split(QLatin1Char('\n'));
    if (m_cursorLine < 0 || m_cursorLine >= lines.size())
        return;
    QString& line = lines[m_cursorLine];
    // Find the start of the current word
    int col = m_cursorColumn;
    int start = col;
    while (start > 0 && (line.at(start - 1).isLetterOrNumber() || line.at(start - 1) == QLatin1Char('_')))
        --start;
    QString word = line.mid(start, col - start);
    if (word != trigger)
        return;
    line.replace(start, col - start, snippet);
    m_doc->setText(lines.join(QLatin1Char('\n')));
    // Position cursor after first tab stop ($1)
    m_cursorColumn = start + snippet.indexOf(QLatin1String("$1"));
    if (m_cursorColumn < 0)
        m_cursorColumn = start + snippet.size();
    emit cursorChanged();
    update();
}

QString PrimoEditor::currentWord() const
{
    if (!m_doc)
        return QString();
    QString txt = m_doc->text();
    QStringList lines = txt.split(QLatin1Char('\n'));
    if (m_cursorLine < 0 || m_cursorLine >= lines.size())
        return QString();
    QString line = lines.at(m_cursorLine);
    int col = qMin(m_cursorColumn, line.size());
    int start = col;
    while (start > 0 && (line.at(start - 1).isLetterOrNumber() || line.at(start - 1) == QLatin1Char('_')))
        --start;
    return line.mid(start, col - start);
}

bool PrimoEditor::canUndo() const
{
    return m_doc && m_doc->canUndo();
}
bool PrimoEditor::canRedo() const
{
    return m_doc && m_doc->canRedo();
}
bool PrimoEditor::isUndoDisabled() const
{
    return m_doc && m_doc->isUndoDisabled();
}
void PrimoEditor::undo()
{
    if (m_readOnly || !m_doc || m_doc->isUndoDisabled())
        return;
    m_doc->undo();
    // Clamp cursor after undo
    QStringList ls = m_doc->text().split(QLatin1Char('\n'));
    if (m_cursorLine >= ls.size())
        m_cursorLine = qMax(0, ls.size() - 1);
    if (m_cursorLine >= 0 && m_cursorColumn > ls[m_cursorLine].size())
        m_cursorColumn = ls[m_cursorLine].size();
    emit cursorChanged();
    m_linesDirty = true;
    m_highlightDirty = true;
    m_highlightVersion++;
    update();
    scheduleHighlight();
}
void PrimoEditor::redo()
{
    if (m_readOnly || !m_doc || m_doc->isUndoDisabled())
        return;
    m_doc->redo();
    QStringList ls = m_doc->text().split(QLatin1Char('\n'));
    if (m_cursorLine >= ls.size())
        m_cursorLine = qMax(0, ls.size() - 1);
    if (m_cursorLine >= 0 && m_cursorColumn > ls[m_cursorLine].size())
        m_cursorColumn = ls[m_cursorLine].size();
    emit cursorChanged();
    m_linesDirty = true;
    m_highlightDirty = true;
    m_highlightVersion++;
    update();
    scheduleHighlight();
}
void PrimoEditor::copy()
{
    if (!m_doc)
        return;
    QString txt = selectedText();
    if (txt.isEmpty())
    {
        // Fallback: copy whole line when nothing selected
        txt = m_doc->lineAt(m_cursorLine);
        if (txt.isEmpty())
            return;
    }
    if (auto* cb = QGuiApplication::clipboard())
        cb->setText(txt);
}
void PrimoEditor::cut()
{
    if (m_readOnly || !m_doc)
        return;
    QString txt = selectedText();
    if (!txt.isEmpty())
    {
        if (auto* cb = QGuiApplication::clipboard())
            cb->setText(txt);
        deleteSelection();
        return;
    }
    // Fallback: cut whole line when nothing selected
    txt = m_doc->lineAt(m_cursorLine);
    if (txt.isEmpty())
        return;
    if (auto* cb = QGuiApplication::clipboard())
        cb->setText(txt);
    QStringList ls = m_doc->text().split(QLatin1Char('\n'));
    if (m_cursorLine < 0 || m_cursorLine >= ls.size())
        return;
    ls.removeAt(m_cursorLine);
    if (ls.isEmpty())
        ls.append(QString());
    if (m_cursorLine >= ls.size())
        m_cursorLine = qMax(0, ls.size() - 1);
    m_cursorColumn = 0;
    m_doc->setText(ls.join(QLatin1Char('\n')));
    emit cursorChanged();
}
void PrimoEditor::paste()
{
    if (m_readOnly || !m_doc)
        return;
    auto* cb = QGuiApplication::clipboard();
    if (!cb)
        return;
    QString txt = cb->text();
    if (txt.isEmpty())
        return;
    // Insert at cursor, split by lines if multi-line paste
    insertAtCursor(txt);
}
void PrimoEditor::selectAll()
{
    if (!m_doc)
        return;
    int lastLine = qMax(0, m_doc->lineCount() - 1);
    QString last = m_doc->lineAt(lastLine);
    m_selAnchorLine = 0;
    m_selAnchorCol = 0;
    m_cursorLine = lastLine;
    m_cursorColumn = last.size();
    emit cursorChanged();
    update();
}

void PrimoEditor::applyThemeColors(const ThemeColors& colors)
{
    m_theme = colors;
    m_bg = colors.background;
    m_fg = colors.foreground;
    emit backgroundColorChanged();
    emit foregroundColorChanged();
    update();
    // Re-highlight with new theme colors
    m_highlightDirty = true;
    m_highlightVersion++;
    scheduleHighlight();
}

void PrimoEditor::setCustomTheme(const QString& name)
{
    m_customTheme = name;
}

QString PrimoEditor::customTheme() const
{
    return m_customTheme;
}

void PrimoEditor::loadFile(const QString& path)
{
    if (!m_doc)
        return;
    QFileInfo fi(path);
    // >50MB read-only + warning
    if (fi.exists() && fi.size() > FileService::WarningFileSize)
    {
        // Check size: Warning 50MB, Max 100MB
        if (fi.size() > 50LL * 1024 * 1024)
        {
            m_readOnly = true;
            emit readOnlyChanged();
        }
    }
    else
    {
        // respect default readOnly false unless previously set?
        // keep as is, but ensure not stuck readOnly from previous large file
        // Reset if previous was large and new is small
        if (fi.size() <= static_cast<qint64>(50) * 1024 * 1024 && m_readOnly)
        {
            // only clear if it was due to large file; keep otherwise? For now clear
            m_readOnly = false;
            emit readOnlyChanged();
        }
    }

    QString error;
    bool ok = m_doc->loadFromFile(path, &error);
    if (ok)
    {
        QString lang = SettingsManager::instance().syntaxForFile(path);
        m_doc->setLanguage(lang);
        // update theme from settings
        m_theme = getThemeColors(SettingsManager::instance().theme());
        m_bg = m_theme.background;
        m_fg = m_theme.foreground;
        m_cursorLine = 0;
        m_cursorColumn = 0;
        emit cursorChanged();
        emit fileLoaded(true, QString());
        // For large files, emit warning error string as info?
        if (fi.size() > static_cast<qint64>(50) * 1024 * 1024)
        {
            // emit as second signal? Use fileLoaded with ok but also log
            // QML will show banner via readOnly
        }
    }
    else
    {
        emit fileLoaded(false, error);
    }
    m_linesDirty = true;
    m_highlightDirty = true;
    m_highlightVersion++;
    m_highlightCache.clear();
    m_contentHeight = contentHeight();
    emit contentHeightChanged();
    scheduleHighlight();
    update();
    setFocus(true);
}

bool PrimoEditor::save()
{
    if (m_readOnly)
    {
        emit fileSaved(false, QStringLiteral("Read-only (large file >50MB)"));
        return false;
    }
    if (!m_doc || m_doc->filePath().isEmpty())
        return false;
    QString error;
    bool ok = m_doc->saveToFile(m_doc->filePath(), &error);
    if (ok)
        m_doc->setModified(false);
    emit fileSaved(ok, error);
    return ok;
}
bool PrimoEditor::saveAs(const QString& path)
{
    if (m_readOnly)
    {
        emit fileSaved(false, QStringLiteral("Read-only"));
        return false;
    }
    if (!m_doc)
        return false;
    QString error;
    bool ok = m_doc->saveToFile(path, &error);
    if (ok)
    {
        m_doc->setFilePath(path);
        m_doc->setModified(false);
    }
    emit fileSaved(ok, error);
    m_linesDirty = true;
    update();
    return ok;
}
void PrimoEditor::setCursorPosition(int line, int column)
{
    bool changed = false;
    if (m_cursorLine != line)
    {
        m_cursorLine = line;
        changed = true;
    }
    if (m_cursorColumn != column)
    {
        m_cursorColumn = column;
        changed = true;
    }
    if (changed)
    {
        emit cursorChanged();
        update();
    }
}
void PrimoEditor::insertAtCursor(const QString& s)
{
    if (m_readOnly)
        return;
    if (!m_doc)
        return;
    QString txt = m_doc->text();
    QStringList lines = txt.split(QLatin1Char('\n'));
    if (m_cursorLine < 0)
        m_cursorLine = 0;
    if (m_cursorLine >= lines.size())
    {
        while (lines.size() <= m_cursorLine)
            lines.append(QString());
    }
    QString& line = lines[m_cursorLine];
    if (m_cursorColumn < 0)
        m_cursorColumn = 0;
    if (m_cursorColumn > line.size())
        m_cursorColumn = line.size();
    line.insert(m_cursorColumn, s);
    m_cursorColumn += s.size();
    m_doc->setText(lines.join(QLatin1Char('\n')));
    emit cursorChanged();
}
void PrimoEditor::backspaceAtCursor()
{
    if (m_readOnly)
        return;
    if (!m_doc)
        return;
    QString txt = m_doc->text();
    QStringList lines = txt.split(QLatin1Char('\n'));
    if (m_cursorLine < 0 || m_cursorLine >= lines.size())
        return;
    QString& line = lines[m_cursorLine];
    if (m_cursorColumn > 0)
    {
        line.remove(m_cursorColumn - 1, 1);
        m_cursorColumn--;
    }
    else if (m_cursorLine > 0)
    {
        int prevLen = lines[m_cursorLine - 1].size();
        lines[m_cursorLine - 1] += line;
        lines.removeAt(m_cursorLine);
        m_cursorLine--;
        m_cursorColumn = prevLen;
        // bookmarks shift
        QSet<int> nb;
        for (int b : m_bookmarks)
        {
            if (b == m_cursorLine + 1)
                nb.insert(m_cursorLine);
            else if (b > m_cursorLine + 1)
                nb.insert(b - 1);
            else
                nb.insert(b);
        }
        m_bookmarks = nb;
    }
    else
        return;
    m_doc->setText(lines.join(QLatin1Char('\n')));
    emit cursorChanged();
}

void PrimoEditor::ensureMetrics()
{
    QFontMetricsF fm(m_font);
    m_lineHeight = fm.height() * 1.35;
    m_charWidth = fm.horizontalAdvance(QLatin1Char('M'));
    if (m_lineHeight < 12)
        m_lineHeight = 12;
    // gutter width based on digits, like CodeEditor::updateLineNumberWidth
    int lineCount = m_doc ? m_doc->lineCount() : 1;
    if (lineCount < 1)
        lineCount = 1;
    int digits = QString::number(lineCount).length();
    if (m_gutterVisible)
    {
        qreal w = fm.horizontalAdvance(QString(digits, QLatin1Char('9'))) + fm.height();
        // extra for bookmark icon
        m_gutterWidth = w + 16;
    }
    else
    {
        m_gutterWidth = 0;
    }
    emit gutterWidthChanged();
    setImplicitHeight(contentHeight());
}

void PrimoEditor::ensureCursorVisibleHorizontally()
{
    if (!m_doc || !window())
        return;
    QFontMetricsF fm(m_font);
    QStringList ls = m_doc->text().split(QLatin1Char('\n'));
    if (m_cursorLine < 0 || m_cursorLine >= ls.size())
        return;
    QString curLine = expandedTab(ls.at(m_cursorLine));
    QString before = curLine.left(qMin(m_cursorColumn, curLine.size()));
    qreal gutterW = m_gutterVisible ? m_gutterWidth : 0;
    qreal cx = gutterW + 8 - m_hScroll + fm.horizontalAdvance(before);
    QQuickWindow* win = window();
    qreal winWidth = win ? win->width() : boundingRect().width();
    qreal visibleWidth = winWidth - gutterW - 16;
    qreal leftEdge = gutterW + 8 - m_hScroll;
    if (cx < leftEdge)
    {
        setHScroll(leftEdge - cx);
    }
    else if (cx + 2 > leftEdge + visibleWidth)
    {
        setHScroll(cx + 2 - leftEdge - visibleWidth + m_hScroll);
    }
}

QList<PrimoEditor::WrapInfo> PrimoEditor::buildWrapInfos(int availGlobal, int* outCursorVisualRow)
{
    QList<WrapInfo> wrapInfos;
    int cursorVisualRow = -1;
    if (!m_doc || availGlobal <= 0)
    {
        if (outCursorVisualRow)
            *outCursorVisualRow = cursorVisualRow;
        return wrapInfos;
    }
    int total = m_doc->lineCount();
    for (int i = 0; i < total; ++i)
    {
        QString line = m_doc->lineAt(i);
        if (line.isEmpty())
        {
            wrapInfos.append({i, 0, 0});
            if (i == m_cursorLine && m_cursorColumn == 0)
                cursorVisualRow = wrapInfos.size() - 1;
        }
        else
        {
            QString disp = expandedTab(line);
            for (int off = 0; off < disp.size(); off += availGlobal)
            {
                int len = qMin(availGlobal, disp.size() - off);
                wrapInfos.append({i, off, len});
                if (i == m_cursorLine && m_cursorColumn >= off && m_cursorColumn < off + len)
                    cursorVisualRow = wrapInfos.size() - 1;
            }
            if (i == m_cursorLine && m_cursorColumn == disp.size())
            {
                for (int v = wrapInfos.size() - 1; v >= 0; --v)
                {
                    if (wrapInfos[v].line == i)
                    {
                        cursorVisualRow = v;
                        break;
                    }
                }
            }
        }
    }
    if (cursorVisualRow == -1 && !wrapInfos.isEmpty())
        cursorVisualRow = 0;
    if (outCursorVisualRow)
        *outCursorVisualRow = cursorVisualRow;
    return wrapInfos;
}

void PrimoEditor::geometryChange(const QRectF& newGeometry, const QRectF& oldGeometry)
{
    QQuickItem::geometryChange(newGeometry, oldGeometry);
    if (newGeometry.size() != oldGeometry.size())
        update();
}

void PrimoEditor::keyPressEvent(QKeyEvent* event)
{
    if (m_readOnly && event->text().size() > 0 && !(event->modifiers() & Qt::ControlModifier))
    {
        // allow navigation but not edits
        if (event->key() != Qt::Key_Left && event->key() != Qt::Key_Right && event->key() != Qt::Key_Up && event->key() != Qt::Key_Down)
        {
            event->ignore();
            return;
        }
    }
    if (event->modifiers() & Qt::ControlModifier && event->key() == Qt::Key_B)
    {
        toggleBookmarkCurrent();
        event->accept();
        return;
    }
    if (event->key() == Qt::Key_F2)
    { // next bookmark? simple toggle for now
        toggleBookmarkCurrent();
        event->accept();
        return;
    }
    if (event->key() == Qt::Key_Backspace)
    {
        backspaceAtCursor();
        event->accept();
        return;
    }
    if (event->key() == Qt::Key_Return || event->key() == Qt::Key_Enter)
    {
        insertAtCursor(QStringLiteral("\n"));
        m_cursorLine++;
        m_cursorColumn = 0;
        emit cursorChanged();
        event->accept();
        return;
    }
    if (event->key() == Qt::Key_Left)
    {
        if (m_cursorColumn > 0)
            m_cursorColumn--;
        else if (m_cursorLine > 0)
        {
            m_cursorLine--;
            QStringList ls = m_doc->text().split(QLatin1Char('\n'));
            if (m_cursorLine < ls.size())
                m_cursorColumn = ls[m_cursorLine].size();
        }
        emit cursorChanged();
        update();
        event->accept();
        return;
    }
    if (event->key() == Qt::Key_Right)
    {
        QStringList ls = m_doc->text().split(QLatin1Char('\n'));
        if (m_cursorLine < ls.size() && m_cursorColumn < ls[m_cursorLine].size())
            m_cursorColumn++;
        else if (m_cursorLine + 1 < ls.size())
        {
            m_cursorLine++;
            m_cursorColumn = 0;
        }
        emit cursorChanged();
        ensureCursorVisibleHorizontally();
        update();
        event->accept();
        return;
    }
    if (event->key() == Qt::Key_Up)
    {
        if (m_cursorLine > 0)
            m_cursorLine--;
        emit cursorChanged();
        update();
        event->accept();
        return;
    }
    if (event->key() == Qt::Key_Down)
    {
        QStringList ls = m_doc->text().split(QLatin1Char('\n'));
        if (m_cursorLine + 1 < ls.size())
            m_cursorLine++;
        emit cursorChanged();
        update();
        event->accept();
        return;
    }
    if (event->modifiers() & Qt::ControlModifier && event->key() == Qt::Key_V)
    {
        paste();
        event->accept();
        return;
    }
    if (event->key() == Qt::Key_Tab)
    {
        // Try snippet expansion first
        QString word = currentWord();
        if (!word.isEmpty())
        {
            // Check if word matches a snippet trigger
            QStringList triggers = {QStringLiteral("for"), QStringLiteral("if"), QStringLiteral("else"),
                                    QStringLiteral("func"), QStringLiteral("fn"), QStringLiteral("class"),
                                    QStringLiteral("main")};
            if (triggers.contains(word))
            {
                insertSnippet(word);
                event->accept();
                return;
            }
        }
        // Fallback: insert tab
        insertAtCursor(QStringLiteral("\t"));
        event->accept();
        return;
    }
    QString t = event->text();
    if (!t.isEmpty() && t.at(0).isPrint() && !(event->modifiers() & Qt::ControlModifier))
    {
        insertAtCursor(t);
        event->accept();
        return;
    }
    QQuickItem::keyPressEvent(event);
}

void PrimoEditor::mousePressEvent(QMouseEvent* event)
{
    setFocus(true);
    qreal gutterW = m_gutterVisible ? m_gutterWidth : 0;
    if (event->position().x() < gutterW)
    {
        // gutter click: toggle bookmark
        int line = int((event->position().y() - 4) / m_lineHeight);
        if (line >= 0)
            toggleBookmark(line);
        event->accept();
        return;
    }
    int line = int((event->position().y() - 4) / m_lineHeight);
    if (line < 0)
        line = 0;
    QStringList ls = m_doc ? m_doc->text().split(QLatin1Char('\n')) : QStringList{QString()};
    if (line >= ls.size())
        line = qMax(0, ls.size() - 1);
    int col = 0;
    if (line < ls.size())
    {
        QString l = expandedTab(ls[line]);
        QFontMetricsF fm(m_font);
        qreal bestDist = 1e9;
        int best = 0;
        qreal targetX = event->position().x() - gutterW - 8;
        if (targetX < 0)
            targetX = 0;
        for (int c = 0; c <= l.size(); ++c)
        {
            qreal w = fm.horizontalAdvance(l.left(c));
            qreal d = qAbs(w - targetX);
            if (d < bestDist)
            {
                bestDist = d;
                best = c;
            }
        }
        col = best;
    }
    m_cursorLine = line;
    m_cursorColumn = col;
    emit cursorChanged();
    update();
    event->accept();
}

void PrimoEditor::focusInEvent(QFocusEvent* e)
{
    QQuickItem::focusInEvent(e);
    update();
}
void PrimoEditor::focusOutEvent(QFocusEvent* e)
{
    QQuickItem::focusOutEvent(e);
    update();
}

QString PrimoEditor::expandedTab(const QString& line) const
{
    if (!line.contains(QLatin1Char('\t')))
        return line;
    QString out;
    out.reserve(line.size() + 8);
    for (QChar ch : line)
    {
        if (ch == QLatin1Char('\t'))
            out += QString(m_tabWidth, QLatin1Char(' '));
        else
            out += ch;
    }
    return out;
}

void PrimoEditor::scheduleHighlight(int firstLine, int lastLine)
{
    if (!m_doc || m_doc->text().isEmpty())
        return;
    // Determine language already set
    QString lang = m_doc->language();
    if (lang.isEmpty())
        lang = m_doc->language(); // fallback

    // Ensure cache sized
    int total = m_doc->lineCount();
    if (m_highlightCache.size() != total)
    {
        m_highlightCache.resize(total);
    }

    // For large files >10000 lines, only highlight visible + 500 around cursor to keep perf
    bool isLarge = (total > 10000 || m_doc->length() > 50 * 1024 * 1024);
    int first = 0, last = total - 1;
    if (firstLine >= 0 && lastLine >= 0)
    {
        first = firstLine;
        last = lastLine;
    }
    else if (isLarge)
    {
        // visible range
        QQuickItem* flick = parentItem();
        qreal contentY = 0;
        qreal h = boundingRect().height();
        if (flick)
        {
            QVariant v = flick->property("contentY");
            if (v.isValid())
                contentY = v.toReal();
            else if (flick->parentItem())
            {
                QVariant v2 = flick->parentItem()->property("contentY");
                if (v2.isValid())
                    contentY = v2.toReal();
            }
            if (h <= 0)
                h = 800;
        }
        first = qMax(0, int(contentY / m_lineHeight) - 100);
        last = qMin(total - 1, first + int(h / m_lineHeight) + 200);
        // Also ensure cursor line highlighted
        first = qMin(first, qMax(0, m_cursorLine - 100));
        last = qMax(last, qMin(total - 1, m_cursorLine + 100));
    }

    // Fast path for small files or visible chunk: sync
    if (!isLarge && (last - first) < 500)
    {
        QStringList chunk;
        chunk.reserve(last - first + 1);
        for (int i = first; i <= last; ++i)
            chunk.append(m_doc->lineAt(i));
        QVector<QVector<QTextLayout::FormatRange>> res;
        res.reserve(chunk.size());
        for (auto& ln : chunk)
            res.append(PrimoHighlighter::formatsForLine(ln, lang, m_theme));
        applyHighlightResult(m_highlightVersion, first, res);
        return;
    }

    // Async for large or bigger chunk
    QStringList chunk;
    chunk.reserve(last - first + 1);
    for (int i = first; i <= last; ++i)
        chunk.append(m_doc->lineAt(i));
    m_highlightController->request(m_highlightVersion, first, chunk, lang, m_theme);
}

void PrimoEditor::applyHighlightResult(int version, int firstLine, const QVector<QVector<QTextLayout::FormatRange>>& fmts)
{
    if (version != m_highlightVersion)
        return;
    if (firstLine < 0 || firstLine >= m_highlightCache.size())
        return;
    for (int i = 0; i < fmts.size() && firstLine + i < m_highlightCache.size(); ++i)
    {
        m_highlightCache[firstLine + i] = fmts[i];
    }
    update();
}

QSGNode* PrimoEditor::updatePaintNode(QSGNode* oldNode, UpdatePaintNodeData* data)
{
    Q_UNUSED(data);
    QSGNode* root = oldNode;
    if (!root)
        root = new QSGNode;
    while (root->firstChild())
        delete root->firstChild();
    QQuickWindow* win = window();
    if (!win)
        return root;
    const QRectF bounds = boundingRect();
    if (bounds.isEmpty())
        return root;

    int totalLines;
    QStringList lines;
    bool useDocLineAt = false;
    if (m_linesDirty || m_linesCache.isEmpty())
    {
        QString txt = m_doc ? m_doc->text() : QString();
        if (txt.isEmpty())
        {
            lines = QStringList{QString()};
        }
        else if (m_doc && m_doc->lineCount() <= MaxCachedLines)
        {
            lines = txt.split(QLatin1Char('\n'));
        }
        else
        {
            // Document too large to cache all lines: compute on demand
            useDocLineAt = true;
        }
        m_linesCache = lines;
        m_linesDirty = false;
        // also resize highlight cache
        int cacheLineCount = useDocLineAt ? 0 : lines.size();
        if (m_highlightCache.size() != cacheLineCount)
        {
            m_highlightCache.resize(cacheLineCount);
            m_highlightDirty = true;
        }
        // Cap highlight cache size
        if (m_highlightCache.size() > MaxCachedLines)
            m_highlightCache.resize(MaxCachedLines);
    }
    else
        lines = m_linesCache;
    totalLines = lines.size();
    qreal needed = totalLines * m_lineHeight + 8;
    if (!qFuzzyCompare(needed, m_contentHeight))
    {
        m_contentHeight = needed;
        emit contentHeightChanged();
        setImplicitHeight(needed);
        ensureMetrics();
    }

    // Background
    {
        QSGRectangleNode* bg = win->createRectangleNode();
        bg->setRect(bounds);
        bg->setColor(m_bg);
        root->appendChildNode(bg);
    }

    // Gutter background
    qreal gutterW = m_gutterVisible ? m_gutterWidth : 0;
    if (m_gutterVisible)
    {
        QSGRectangleNode* gutterBg = win->createRectangleNode();
        gutterBg->setRect(QRectF(0, 0, gutterW, bounds.height()));
        // slightly darker than bg
        QColor gb = m_theme.isDark ? m_bg.darker(115) : m_bg.darker(103);
        // use marginBg if available
        gb = m_theme.marginBg;
        gutterBg->setColor(gb);
        root->appendChildNode(gutterBg);
        // gutter separator line (1px)
        QSGRectangleNode* sep = win->createRectangleNode();
        sep->setRect(QRectF(gutterW - 1, 0, 1, bounds.height()));
        sep->setColor(m_theme.border);
        root->appendChildNode(sep);
    }

    // Determine visible range via Flickable contentY
    int firstLine = 0;
    int lastLine = qMin(totalLines - 1, int(ceil(bounds.height() / m_lineHeight)) + 1);
    QQuickItem* flick = parentItem();
    qreal contentY = 0;
    if (flick)
    {
        QVariant v = flick->property("contentY");
        if (v.isValid())
            contentY = v.toReal();
        else if (flick->parentItem())
        {
            QVariant v2 = flick->parentItem()->property("contentY");
            if (v2.isValid())
                contentY = v2.toReal();
        }
    }
    if (contentY > 0)
    {
        firstLine = int(contentY / m_lineHeight);
        lastLine = qMin(totalLines - 1, firstLine + int(ceil(bounds.height() / m_lineHeight)) + 2);
        if (firstLine < 0)
            firstLine = 0;
    }

    // Schedule async highlight for visible range if dirty
    if (m_highlightDirty)
    {
        scheduleHighlight(firstLine, lastLine);
        m_highlightDirty = false;
    }
    else
    {
        // also ensure visible lines highlighted (if cache empty)
        bool need = false;
        for (int i = firstLine; i <= lastLine && i < m_highlightCache.size(); ++i)
            if (m_highlightCache[i].isEmpty() && !lines[i].isEmpty())
            {
                need = true;
                break;
            }
        if (need)
            scheduleHighlight(firstLine, lastLine);
    }

    QFontMetricsF fm(m_font);
    const qreal leftPad = gutterW + 8;
    const qreal textLeft = leftPad - m_hScroll;

    // Word wrap: build wrap infos and compute visible visual-row range
    int cursorVisualRow = -1;
    QList<WrapInfo> wrapInfos;
    int totalWrapRows = 0;
    if (m_wordWrap)
    {
        int availGlobal = qMax(1, int(bounds.width() - gutterW - 16));
        wrapInfos = buildWrapInfos(availGlobal, &cursorVisualRow);
        totalWrapRows = wrapInfos.size();
    }
    else
    {
        totalWrapRows = totalLines;
    }

    // Determine visible range
    int firstVisualRow = 0;
    int lastVisualRow = qMin(totalWrapRows - 1, int(ceil(bounds.height() / m_lineHeight)) + 1);
    if (m_wordWrap)
    {
        firstVisualRow = int(m_wrapScrollTop / m_lineHeight);
        lastVisualRow = qMin(totalWrapRows - 1, firstVisualRow + int(ceil(bounds.height() / m_lineHeight)) + 2);
        if (firstVisualRow < 0)
            firstVisualRow = 0;
        // Ensure cursor visual row is visible
        if (cursorVisualRow >= 0 && (cursorVisualRow < firstVisualRow || cursorVisualRow > lastVisualRow))
        {
            setWrapScrollTop(cursorVisualRow * m_lineHeight);
            firstVisualRow = cursorVisualRow;
            lastVisualRow = qMin(totalWrapRows - 1, firstVisualRow + int(ceil(bounds.height() / m_lineHeight)) + 2);
        }
    }
    else
    {
        if (contentY > 0)
        {
            firstVisualRow = int(contentY / m_lineHeight);
            lastVisualRow = qMin(totalWrapRows - 1, firstVisualRow + int(ceil(bounds.height() / m_lineHeight)) + 2);
            if (firstVisualRow < 0)
                firstVisualRow = 0;
        }
    }

    // Current line highlight
    if (!m_wordWrap && m_cursorLine >= firstVisualRow && m_cursorLine <= lastVisualRow)
    {
        QSGRectangleNode* curBg = win->createRectangleNode();
        curBg->setRect(QRectF(gutterW, 4 + m_cursorLine * m_lineHeight, bounds.width() - gutterW, m_lineHeight));
        QColor hl = m_theme.lineHighlight;
        hl.setAlpha(180);
        curBg->setColor(hl);
        root->appendChildNode(curBg);
    }
    else if (m_wordWrap && cursorVisualRow >= 0 && cursorVisualRow >= firstVisualRow && cursorVisualRow <= lastVisualRow)
    {
        QSGRectangleNode* curBg = win->createRectangleNode();
        curBg->setRect(QRectF(gutterW, 4 + (cursorVisualRow - firstVisualRow) * m_lineHeight, bounds.width() - gutterW, m_lineHeight));
        QColor hl = m_theme.lineHighlight;
        hl.setAlpha(180);
        curBg->setColor(hl);
        root->appendChildNode(curBg);
    }

    // Gutter numbers + bookmarks + diagnostics (only in non-wrap mode for now)
    if (m_gutterVisible && !m_wordWrap)
    {
        for (int i = firstVisualRow; i <= lastVisualRow; ++i)
        {
            qreal y = 4 + i * m_lineHeight;
            // Bookmark marker
            if (m_bookmarks.contains(i))
            {
                QSGRectangleNode* bm = win->createRectangleNode();
                bm->setRect(QRectF(4, y + (m_lineHeight - 8) / 2, 8, 8));
                bm->setColor(QColor(255, 160, 0));
                root->appendChildNode(bm);
            }
            // Diagnostics dot
            if (m_diagLines.contains(i))
            {
                QSGRectangleNode* diag = win->createRectangleNode();
                diag->setRect(QRectF(gutterW - 10, y + (m_lineHeight - 6) / 2, 6, 6));
                diag->setColor(QColor(255, 0, 0));
                root->appendChildNode(diag);
            }
            // Line number
            if (m_showLineNumbers)
            {
                QString numStr = QString::number(i + 1);
                QTextLayout numLayout(numStr, m_font);
                numLayout.beginLayout();
                QTextLine tl = numLayout.createLine();
                tl.setLineWidth(gutterW - 20);
                numLayout.endLayout();
                QSGTextNode* numNode = win->createTextNode();
                QColor nc = (i == m_cursorLine) ? m_theme.foreground : m_theme.marginFg;
                numNode->setColor(nc);
                numNode->setFiltering(QSGTexture::Linear);
                qreal nx = gutterW - fm.horizontalAdvance(numStr) - 8;
                numNode->addTextLayout(QPointF(nx, y), &numLayout);
                root->appendChildNode(numNode);
            }
        }
    }

    // Text rendering
    if (m_wordWrap)
    {
        for (int v = firstVisualRow; v <= lastVisualRow && v < wrapInfos.size(); ++v)
        {
            const WrapInfo& wi = wrapInfos[v];
            int lineIdx = wi.line;
            qreal y = 4 + (v - firstVisualRow) * m_lineHeight;
            QString raw = (lineIdx < lines.size()) ? lines.at(lineIdx) : (m_doc ? m_doc->lineAt(lineIdx) : QString());
            QString disp = expandedTab(raw).mid(wi.startCol, wi.len);
            if (disp.isEmpty())
                continue;
            QTextLayout layout(disp, m_font);
            if (wi.line < m_highlightCache.size() && !m_highlightCache[wi.line].isEmpty())
                layout.setFormats(m_highlightCache[wi.line]);
            layout.beginLayout();
            QTextLine tl = layout.createLine();
            tl.setLineWidth(bounds.width() - textLeft - 4);
            layout.endLayout();
            QSGTextNode* textNode = win->createTextNode();
            textNode->setColor(m_fg);
            textNode->setFiltering(QSGTexture::Linear);
            textNode->addTextLayout(QPointF(textLeft, y), &layout);
            root->appendChildNode(textNode);
            // Cursor for wrap mode
            if (hasActiveFocus() && v == cursorVisualRow)
            {
                QString curLineStr = expandedTab(raw);
                QString before = curLineStr.left(wi.startCol + qMin(m_cursorColumn - wi.startCol, wi.len));
                qreal cx = textLeft + fm.horizontalAdvance(before);
                qreal cy = y;
                QSGGeometryNode* grad =
                    createGradientRectNode(QRectF(cx, cy, 2, m_lineHeight - 2), QColor(0xc4, 0xa7, 0xe7, 200), QColor(0x89, 0xb4, 0xfa, 200), 0.95f);
                root->appendChildNode(grad);
            }
        }
    }
    else
    {
        // Current line highlight already done above for non-wrap
        // Gutter numbers already done above for non-wrap
        // Text lines with syntax highlight
        for (int i = firstVisualRow; i <= lastVisualRow; ++i)
        {
            QString raw;
            if (i < lines.size())
                raw = lines.at(i);
            else if (m_doc)
                raw = m_doc->lineAt(i);
            else
                raw = QString();
            QString disp = expandedTab(raw);
            if (disp.isEmpty())
                continue;
            QTextLayout layout(disp, m_font);
            if (i < m_highlightCache.size() && !m_highlightCache[i].isEmpty())
                layout.setFormats(m_highlightCache[i]);
            layout.beginLayout();
            QTextLine tl = layout.createLine();
            tl.setLineWidth(bounds.width() - textLeft - 4);
            layout.endLayout();
            QSGTextNode* textNode = win->createTextNode();
            textNode->setColor(m_fg);
            textNode->setFiltering(QSGTexture::Linear);
            textNode->addTextLayout(QPointF(textLeft, 4 + i * m_lineHeight), &layout);
            root->appendChildNode(textNode);
        }

        // Cursor with gradient shader (non-wrap mode)
        if (hasActiveFocus() && m_cursorLine >= firstVisualRow && m_cursorLine <= lastVisualRow)
        {
            QString curLineStr;
            if (m_cursorLine < lines.size())
                curLineStr = expandedTab(lines.at(m_cursorLine));
            else if (m_doc)
                curLineStr = expandedTab(m_doc->lineAt(m_cursorLine));
            QString before = curLineStr.left(qMin(m_cursorColumn, curLineStr.size()));
            qreal cx = textLeft + fm.horizontalAdvance(before);
            qreal cy = 4 + m_cursorLine * m_lineHeight;
            QSGGeometryNode* grad =
                createGradientRectNode(QRectF(cx, cy, 2, m_lineHeight - 2), QColor(0xc4, 0xa7, 0xe7, 200), QColor(0x89, 0xb4, 0xfa, 200), 0.95f);
            root->appendChildNode(grad);
        }
    }

    // Read-only overlay hint? handled in QML banner, not here

    // Search match highlighting
    if (!m_searchMatches.isEmpty())
    {
        // Compute line start offsets
        QVector<int> lineOffsets;
        lineOffsets.append(0);
        int offset = 0;
        for (int i = 0; i < totalLines; ++i)
        {
            QString line = (i < lines.size()) ? lines.at(i) : (m_doc ? m_doc->lineAt(i) : QString());
            offset += line.size() + 1; // +1 for newline
            lineOffsets.append(offset);
        }
        for (const auto& match : m_searchMatches)
        {
            int matchPos = match.first;
            int matchLen = match.second;
            // Find which line this match is on
            int matchLine = -1;
            int matchCol = -1;
            for (int i = 0; i < totalLines; ++i)
            {
                int lineEnd = lineOffsets.value(i + 1, offset);
                if (matchPos >= lineOffsets[i] && matchPos < lineEnd)
                {
                    matchLine = i;
                    matchCol = matchPos - lineOffsets[i];
                    break;
                }
            }
            if (matchLine < 0)
                continue;
            // Check visibility
            bool visible = false;
            qreal matchY = 0;
            if (m_wordWrap)
            {
                // Find which visual row contains this match
                int availGlobal = qMax(1, int(bounds.width() - gutterW - 16));
                int visualRow = 0;
                for (int i = 0; i < matchLine; ++i)
                {
                    QString line = (i < lines.size()) ? lines.at(i) : (m_doc ? m_doc->lineAt(i) : QString());
                    if (line.isEmpty())
                        ++visualRow;
                    else
                    {
                        QString disp = expandedTab(line);
                        visualRow += (disp.size() + availGlobal - 1) / qMax(1, availGlobal);
                    }
                }
                // Find which segment of this line contains the match column
                QString lineText = (matchLine < lines.size()) ? lines.at(matchLine) : (m_doc ? m_doc->lineAt(matchLine) : QString());
                QString disp = expandedTab(lineText);
                int segStart = 0;
                while (segStart < disp.size())
                {
                    int segLen = qMin(availGlobal, disp.size() - segStart);
                    if (matchCol >= segStart && matchCol < segStart + segLen)
                    {
                        visible = (visualRow >= firstVisualRow && visualRow <= lastVisualRow);
                        matchY = 4 + (visualRow - firstVisualRow) * m_lineHeight;
                        break;
                    }
                    segStart += segLen;
                    ++visualRow;
                }
                if (!visible)
                    continue;
            }
            else
            {
                visible = (matchLine >= firstVisualRow && matchLine <= lastVisualRow);
                matchY = 4 + matchLine * m_lineHeight;
                if (!visible)
                    continue;
            }
            // Compute X range of the match
            QString lineText = (matchLine < lines.size()) ? lines.at(matchLine) : (m_doc ? m_doc->lineAt(matchLine) : QString());
            QString disp = expandedTab(lineText);
            QString before = disp.left(qMin(matchCol, disp.size()));
            QString matchText = disp.mid(qMin(matchCol, disp.size()), qMin(matchLen, disp.size() - qMin(matchCol, disp.size())));
            qreal startX = textLeft + fm.horizontalAdvance(before);
            qreal endX = startX + fm.horizontalAdvance(matchText);
            QSGRectangleNode* hl = win->createRectangleNode();
            hl->setRect(QRectF(startX, matchY, endX - startX, m_lineHeight - 2));
            QColor hlColor = m_theme.lineHighlight;
            hlColor.setAlpha(100);
            hl->setColor(hlColor);
            root->appendChildNode(hl);
        }
    }

    return root;
}
