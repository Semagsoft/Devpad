/*
 * Devpad - A C++/Qt6 code editor
 * Copyright (C) 2026 Semagsoft
 */

#include "primoeditor.h"

#include "core/fileservice.h"
#include "managers/settingsmanager.h"
#include "primoshader.h"
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
        update();
    }
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
    if (m_linesDirty)
    {
        QString txt = m_doc ? m_doc->text() : QString();
        int lines = txt.isEmpty() ? 1 : txt.count(QLatin1Char('\n')) + 1;
        return lines * m_lineHeight + 8;
    }
    int lines = m_linesCache.isEmpty() ? (m_doc && !m_doc->text().isEmpty() ? m_doc->text().count(QLatin1Char('\n')) + 1 : 1) : m_linesCache.size();
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
    QString line = m_doc->lineAt(m_cursorLine);
    if (line.isEmpty())
        return;
    if (auto* cb = QGuiApplication::clipboard())
        cb->setText(line);
}
void PrimoEditor::cut()
{
    if (m_readOnly || !m_doc)
        return;
    QString line = m_doc->lineAt(m_cursorLine);
    if (auto* cb = QGuiApplication::clipboard())
        cb->setText(line);
    // Remove line
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
    // For QSG, selectAll is visual placeholder: move cursor to end and emit
    if (!m_doc)
        return;
    m_cursorLine = qMax(0, m_doc->lineCount() - 1);
    QString last = m_doc->lineAt(m_cursorLine);
    m_cursorColumn = last.size();
    emit cursorChanged();
    update();
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
        if (fi.size() <= 50 * 1024 * 1024 && m_readOnly)
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
        if (fi.size() > 50 * 1024 * 1024)
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
    if (m_linesDirty || m_linesCache.isEmpty())
    {
        QString txt = m_doc ? m_doc->text() : QString();
        if (txt.isEmpty())
            lines = QStringList{QString()};
        else
            lines = txt.split(QLatin1Char('\n'));
        m_linesCache = lines;
        m_linesDirty = false;
        // also resize highlight cache
        if (m_highlightCache.size() != lines.size())
        {
            m_highlightCache.resize(lines.size());
            m_highlightDirty = true;
        }
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
    const qreal textLeft = leftPad;

    // Current line highlight (subtle) with gradient shader? Use flat for now, gradient node for selection
    if (m_cursorLine >= firstLine && m_cursorLine <= lastLine)
    {
        QSGRectangleNode* curBg = win->createRectangleNode();
        curBg->setRect(QRectF(gutterW, 4 + m_cursorLine * m_lineHeight, bounds.width() - gutterW, m_lineHeight));
        QColor hl = m_theme.lineHighlight;
        hl.setAlpha(180);
        curBg->setColor(hl);
        root->appendChildNode(curBg);
    }

    // Gutter numbers + bookmarks + diagnostics
    if (m_gutterVisible)
    {
        for (int i = firstLine; i <= lastLine; ++i)
        {
            qreal y = 4 + i * m_lineHeight;
            // Bookmark marker (orange dot) like CodeEditor::MARKER_BOOKMARK
            if (m_bookmarks.contains(i))
            {
                QSGRectangleNode* bm = win->createRectangleNode();
                // small square 8x8 centered vertically
                bm->setRect(QRectF(4, y + (m_lineHeight - 8) / 2, 8, 8));
                bm->setColor(QColor(255, 160, 0));
                root->appendChildNode(bm);
            }
            // Diagnostics dot on right side of gutter
            if (m_diagLines.contains(i))
            {
                QSGRectangleNode* diag = win->createRectangleNode();
                diag->setRect(QRectF(gutterW - 10, y + (m_lineHeight - 6) / 2, 6, 6));
                diag->setColor(QColor(255, 0, 0));
                root->appendChildNode(diag);
            }
            // Line number
            QString numStr;
            if (m_relativeNumbers && hasActiveFocus())
            {
                int rel = qAbs(i - m_cursorLine);
                if (i == m_cursorLine)
                    numStr = QString::number(i + 1);
                else
                    numStr = QString::number(rel);
            }
            else
            {
                numStr = QString::number(i + 1);
            }
            QTextLayout numLayout(numStr, m_font);
            numLayout.beginLayout();
            QTextLine tl = numLayout.createLine();
            tl.setLineWidth(gutterW - 20);
            numLayout.endLayout();
            QSGTextNode* numNode = win->createTextNode();
            // active line brighter
            QColor nc = (i == m_cursorLine) ? m_theme.foreground : m_theme.marginFg;
            numNode->setColor(nc);
            numNode->setFiltering(QSGTexture::Linear);
            qreal nx = gutterW - fm.horizontalAdvance(numStr) - 8;
            // if bookmark, shift number a bit? keep
            numNode->addTextLayout(QPointF(nx, y), &numLayout);
            root->appendChildNode(numNode);
        }
    }

    // Text lines with syntax highlight
    for (int i = firstLine; i <= lastLine; ++i)
    {
        QString raw = lines.at(i);
        QString disp = expandedTab(raw);
        if (disp.isEmpty())
            continue;
        QTextLayout layout(disp, m_font);
        // Apply syntax formats if available
        if (i < m_highlightCache.size() && !m_highlightCache[i].isEmpty())
        {
            layout.setFormats(m_highlightCache[i]);
        }
        layout.beginLayout();
        QTextLine tl = layout.createLine();
        tl.setLineWidth(bounds.width() - textLeft - 4);
        layout.endLayout();
        QSGTextNode* textNode = win->createTextNode();
        // default color already via formats, but set base
        textNode->setColor(m_fg);
        textNode->setFiltering(QSGTexture::Linear);
        textNode->addTextLayout(QPointF(textLeft, 4 + i * m_lineHeight), &layout);
        root->appendChildNode(textNode);
    }

    // Cursor with gradient shader (true GLSL) – use gradient rect node for glow
    if (hasActiveFocus() && m_cursorLine >= firstLine && m_cursorLine <= lastLine)
    {
        QString curLineStr = (m_cursorLine < lines.size()) ? expandedTab(lines.at(m_cursorLine)) : QString();
        QString before = curLineStr.left(qMin(m_cursorColumn, curLineStr.size()));
        qreal cx = textLeft + fm.horizontalAdvance(before);
        qreal cy = 4 + m_cursorLine * m_lineHeight;
        // Use gradient shader for cursor glow
        QSGGeometryNode* grad =
            createGradientRectNode(QRectF(cx, cy, 2, m_lineHeight - 2), QColor(0xc4, 0xa7, 0xe7, 200), QColor(0x89, 0xb4, 0xfa, 200), 0.95f);
        // Fallback if shader not available: the node already has material, but we also add simple rect as backup
        root->appendChildNode(grad);
    }

    // Read-only overlay hint? handled in QML banner, not here

    return root;
}
