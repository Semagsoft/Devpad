/*
 * Devpad - A C++/Qt6 code editor
 * Copyright (C) 2026 Semagsoft
 */

#include "primodocument.h"

#include "core/fileservice.h"

#include <QFileInfo>
#include <QUndoCommand>

namespace
{
constexpr qint64 UNDO_DISABLE_BYTES = 50LL * 1024 * 1024;

// Diff-based undo command: stores only the changed range instead of full
// document copies. Memory: O(change_size) instead of O(document_size).
class TextEditCommand : public QUndoCommand
{
public:
    TextEditCommand(PrimoDocument* doc, int position, const QString& removed, const QString& inserted)
        : m_doc(doc), m_position(position), m_removed(removed), m_inserted(inserted)
    {
    }
    void undo() override
    {
        if (!m_doc)
            return;
        // Undo: remove inserted text, put removed text back at same position.
        QString text = m_doc->text();
        text.remove(m_position, m_inserted.size());
        text.insert(m_position, m_removed);
        m_doc->setTextWithoutUndo(text);
    }
    void redo() override
    {
        if (!m_doc)
            return;
        // Redo: remove removed text, put inserted text back at same position.
        QString text = m_doc->text();
        text.remove(m_position, m_removed.size());
        text.insert(m_position, m_inserted);
        m_doc->setTextWithoutUndo(text);
    }

private:
    PrimoDocument* m_doc = nullptr;
    int m_position = 0;
    QString m_removed;
    QString m_inserted;
};
} // namespace

PrimoDocument::PrimoDocument(QObject* parent) : QObject(parent)
{
    m_undoStack = new QUndoStack(this);
    connect(m_undoStack, &QUndoStack::canUndoChanged, this, &PrimoDocument::canUndoChanged);
    connect(m_undoStack, &QUndoStack::canRedoChanged, this, &PrimoDocument::canRedoChanged);
}

PrimoDocument::PrimoDocument(const QString& filePath, const QString& text, const QString& language, QObject* parent)
    : QObject(parent), m_filePath(filePath), m_text(text), m_language(language)
{
    m_undoStack = new QUndoStack(this);
    connect(m_undoStack, &QUndoStack::canUndoChanged, this, &PrimoDocument::canUndoChanged);
    connect(m_undoStack, &QUndoStack::canRedoChanged, this, &PrimoDocument::canRedoChanged);
    invalidateCache();
    if (m_text.size() > UNDO_DISABLE_BYTES)
    {
        m_undoDisabled = true;
        emit undoDisabledChanged();
    }
}

void PrimoDocument::invalidateCache()
{
    m_offsetsDirty = true;
    m_linesDirty = true;
}

void PrimoDocument::rebuildOffsets() const
{
    if (!m_offsetsDirty)
        return;
    m_lineOffsets.clear();
    m_lineOffsets.reserve(m_text.count(QLatin1Char('\n')) + 2);
    m_lineOffsets.append(0);
    for (int i = 0; i < m_text.size(); ++i)
    {
        if (m_text.at(i) == QLatin1Char('\n'))
            m_lineOffsets.append(i + 1);
    }
    m_offsetsDirty = false;
}

void PrimoDocument::pushUndo(int position, const QString& removed, const QString& inserted)
{
    if (m_inUndoRedo || m_undoDisabled)
        return;
    // Avoid pushing no-op
    if (removed.isEmpty() && inserted.isEmpty())
        return;
    m_undoStack->push(new TextEditCommand(this, position, removed, inserted));
}

// Compute the diff between old and new text and push a diff-based undo command.
// Finds common prefix/suffix to minimize the stored change range.
void PrimoDocument::pushUndoDiff(const QString& oldText, const QString& newText)
{
    if (oldText == newText)
        return;

    int minLen = qMin(oldText.size(), newText.size());

    // Common prefix
    int prefix = 0;
    while (prefix < minLen && oldText[prefix] == newText[prefix])
        ++prefix;

    // Common suffix
    int suffix = 0;
    while (suffix < minLen - prefix && oldText[oldText.size() - 1 - suffix] == newText[newText.size() - 1 - suffix])
        ++suffix;

    int oldMiddleLen = oldText.size() - prefix - suffix;
    int newMiddleLen = newText.size() - prefix - suffix;

    QString removed = oldText.mid(prefix, oldMiddleLen);
    QString inserted = newText.mid(prefix, newMiddleLen);

    pushUndo(prefix, removed, inserted);
}

QString PrimoDocument::text() const
{
    return m_text;
}

void PrimoDocument::setText(const QString& text)
{
    if (m_text == text)
        return;
    QString old = m_text;
    bool wasInUndo = m_inUndoRedo;
    if (!wasInUndo)
    {
        // Check disable threshold
        if (text.size() > UNDO_DISABLE_BYTES || old.size() > UNDO_DISABLE_BYTES)
        {
            if (!m_undoDisabled)
            {
                m_undoDisabled = true;
                emit undoDisabledChanged();
                m_undoStack->clear();
            }
        }
        else if (!m_undoDisabled)
        {
            m_text = text;
            m_modified = true;
            invalidateCache();
            emit textChanged();
            emit modifiedChanged();
            emit canUndoChanged();
            emit canRedoChanged();
            // Push diff-based undo command (not full snapshots)
            pushUndoDiff(old, text);
            return;
        }
    }
    // Either in undo/redo or disabled or wasInUndo
    m_text = text;
    m_modified = true;
    invalidateCache();
    emit textChanged();
    emit modifiedChanged();
    if (!wasInUndo)
    {
        emit canUndoChanged();
        emit canRedoChanged();
    }
}

void PrimoDocument::setTextWithoutUndo(const QString& text)
{
    if (m_text == text)
        return;
    m_text = text;
    m_modified = true;
    invalidateCache();
    emit textChanged();
    emit modifiedChanged();
}

void PrimoDocument::setFilePath(const QString& path)
{
    if (m_filePath == path)
        return;
    m_filePath = path;
    emit filePathChanged();
}

QString PrimoDocument::filePath() const
{
    return m_filePath;
}

QString PrimoDocument::language() const
{
    return m_language;
}

void PrimoDocument::setLanguage(const QString& lang)
{
    if (m_language == lang)
        return;
    m_language = lang;
    emit languageChanged();
}

bool PrimoDocument::isModified() const
{
    return m_modified;
}

void PrimoDocument::setModified(bool m)
{
    if (m_modified == m)
        return;
    m_modified = m;
    emit modifiedChanged();
}

int PrimoDocument::lineCount() const
{
    if (m_text.isEmpty())
        return 1;
    rebuildOffsets();
    return m_lineOffsets.size();
}

QStringList PrimoDocument::lines() const
{
    if (!m_linesDirty)
        return m_linesCache;
    rebuildOffsets();
    m_linesCache.clear();
    m_linesCache.reserve(m_lineOffsets.size());
    for (int i = 0; i < m_lineOffsets.size(); ++i)
    {
        int start = m_lineOffsets.at(i);
        int end = (i + 1 < m_lineOffsets.size()) ? m_lineOffsets.at(i + 1) - 1 : m_text.size();
        m_linesCache.append(m_text.mid(start, end - start));
    }
    m_linesDirty = false;
    return m_linesCache;
}

QString PrimoDocument::lineAt(int line) const
{
    rebuildOffsets();
    if (line < 0 || line >= m_lineOffsets.size())
        return {};
    int start = m_lineOffsets.at(line);
    int end = (line + 1 < m_lineOffsets.size()) ? m_lineOffsets.at(line + 1) - 1 : m_text.size();
    return m_text.mid(start, end - start);
}

void PrimoDocument::setLine(int line, const QString& content)
{
    rebuildOffsets();
    if (line < 0 || line >= m_lineOffsets.size())
        return;
    int start = m_lineOffsets.at(line);
    int end = (line + 1 < m_lineOffsets.size()) ? m_lineOffsets.at(line + 1) - 1 : m_text.size();
    QString removed = m_text.mid(start, end - start);
    m_text.replace(start, end - start, content);
    invalidateCache();
    m_modified = true;
    emit textChanged();
    emit modifiedChanged();
    pushUndo(start, removed, content);
}

void PrimoDocument::insertText(int position, const QString& t)
{
    if (position < 0)
        position = 0;
    if (position > m_text.size())
        position = m_text.size();
    m_text.insert(position, t);
    invalidateCache();
    m_modified = true;
    emit textChanged();
    emit modifiedChanged();
    pushUndo(position, QString(), t);
}

void PrimoDocument::removeText(int position, int length)
{
    if (position < 0 || length <= 0 || position >= m_text.size())
        return;
    QString removed = m_text.mid(position, length);
    m_text.remove(position, length);
    invalidateCache();
    m_modified = true;
    emit textChanged();
    emit modifiedChanged();
    pushUndo(position, removed, QString());
}

int PrimoDocument::length() const
{
    return m_text.size();
}

int PrimoDocument::lineStartOffset(int line) const
{
    rebuildOffsets();
    if (line < 0 || line >= m_lineOffsets.size())
        return -1;
    return m_lineOffsets.at(line);
}

QStringList PrimoDocument::visibleLines(int firstLine, int count) const
{
    rebuildOffsets();
    if (firstLine < 0)
        firstLine = 0;
    if (count <= 0)
        return {};
    int end = qMin(firstLine + count, m_lineOffsets.size());
    QStringList out;
    out.reserve(end - firstLine);
    for (int i = firstLine; i < end; ++i)
        out.append(lineAt(i));
    return out;
}

bool PrimoDocument::canUndo() const
{
    return !m_undoDisabled && m_undoStack && m_undoStack->canUndo();
}
bool PrimoDocument::canRedo() const
{
    return !m_undoDisabled && m_undoStack && m_undoStack->canRedo();
}
bool PrimoDocument::isUndoDisabled() const
{
    return m_undoDisabled;
}
void PrimoDocument::setUndoDisabled(bool d)
{
    if (m_undoDisabled != d)
    {
        m_undoDisabled = d;
        emit undoDisabledChanged();
        if (d)
        {
            m_undoStack->clear();
            emit canUndoChanged();
            emit canRedoChanged();
        }
    }
}
void PrimoDocument::undo()
{
    if (m_undoDisabled || !m_undoStack || !m_undoStack->canUndo())
        return;
    m_inUndoRedo = true;
    m_undoStack->undo();
    m_inUndoRedo = false;
    invalidateCache();
    emit textChanged();
    emit modifiedChanged();
}
void PrimoDocument::redo()
{
    if (m_undoDisabled || !m_undoStack || !m_undoStack->canRedo())
        return;
    m_inUndoRedo = true;
    m_undoStack->redo();
    m_inUndoRedo = false;
    invalidateCache();
    emit textChanged();
    emit modifiedChanged();
}
void PrimoDocument::clearUndoStack()
{
    if (m_undoStack)
        m_undoStack->clear();
    emit canUndoChanged();
    emit canRedoChanged();
}

bool PrimoDocument::loadFromFile(const QString& path, QString* error)
{
    FileLoadResult res = FileService::load(path);
    if (!res.ok)
    {
        if (error)
            *error = res.error;
        return false;
    }
    QString old = m_text;
    m_filePath = path;
    m_text = res.text;
    m_modified = false;
    bool shouldDisable = (m_text.size() > UNDO_DISABLE_BYTES) || (QFileInfo(path).size() > UNDO_DISABLE_BYTES);
    if (shouldDisable != m_undoDisabled)
    {
        m_undoDisabled = shouldDisable;
        emit undoDisabledChanged();
    }
    if (m_undoStack)
        m_undoStack->clear();
    invalidateCache();
    emit filePathChanged();
    emit textChanged();
    emit modifiedChanged();
    emit canUndoChanged();
    emit canRedoChanged();
    Q_UNUSED(old);
    return true;
}

bool PrimoDocument::saveToFile(const QString& path, QString* error) const
{
    QString err;
    bool ok = FileService::save(path, m_text, QStringLiteral("UTF-8"), &err);
    if (!ok && error)
        *error = err;
    return ok;
}
