/*
 * Devpad - A C++/Qt6 code editor
 * Copyright (C) 2026 Semagsoft
 */

#include "primodocument.h"

#include "core/fileservice.h"

#include <QFileInfo>

PrimoDocument::PrimoDocument(QObject* parent) : QObject(parent) {}

PrimoDocument::PrimoDocument(const QString& filePath, const QString& text, const QString& language, QObject* parent)
    : QObject(parent), m_filePath(filePath), m_text(text), m_language(language)
{
    invalidateCache();
}

void PrimoDocument::invalidateCache()
{
    m_offsetsDirty = true;
    m_linesDirty = true;
}

void PrimoDocument::rebuildOffsets() const
{
    if (!m_offsetsDirty) return;
    m_lineOffsets.clear();
    m_lineOffsets.reserve(m_text.count(QLatin1Char('\n')) + 2);
    m_lineOffsets.append(0);
    for (int i = 0; i < m_text.size(); ++i) {
        if (m_text.at(i) == QLatin1Char('\n'))
            m_lineOffsets.append(i + 1);
    }
    m_offsetsDirty = false;
}

QString PrimoDocument::text() const { return m_text; }

void PrimoDocument::setText(const QString& text)
{
    if (m_text == text) return;
    m_text = text;
    m_modified = true;
    invalidateCache();
    emit textChanged();
    emit modifiedChanged();
}

QString PrimoDocument::filePath() const { return m_filePath; }

void PrimoDocument::setFilePath(const QString& path)
{
    if (m_filePath == path) return;
    m_filePath = path;
    emit filePathChanged();
}

QString PrimoDocument::language() const { return m_language; }

void PrimoDocument::setLanguage(const QString& lang)
{
    if (m_language == lang) return;
    m_language = lang;
    emit languageChanged();
}

bool PrimoDocument::isModified() const { return m_modified; }

void PrimoDocument::setModified(bool m)
{
    if (m_modified == m) return;
    m_modified = m;
    emit modifiedChanged();
}

int PrimoDocument::lineCount() const
{
    if (m_text.isEmpty()) return 1;
    rebuildOffsets();
    return m_lineOffsets.size();
}

QStringList PrimoDocument::lines() const
{
    if (!m_linesDirty) return m_linesCache;
    rebuildOffsets();
    m_linesCache.clear();
    m_linesCache.reserve(m_lineOffsets.size());
    for (int i = 0; i < m_lineOffsets.size(); ++i) {
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
    if (line < 0 || line >= m_lineOffsets.size()) return {};
    int start = m_lineOffsets.at(line);
    int end = (line + 1 < m_lineOffsets.size()) ? m_lineOffsets.at(line + 1) - 1 : m_text.size();
    return m_text.mid(start, end - start);
}

void PrimoDocument::setLine(int line, const QString& content)
{
    rebuildOffsets();
    if (line < 0 || line >= m_lineOffsets.size()) return;
    int start = m_lineOffsets.at(line);
    int end = (line + 1 < m_lineOffsets.size()) ? m_lineOffsets.at(line + 1) - 1 : m_text.size();
    // keep newline
    m_text.replace(start, end - start, content);
    invalidateCache();
    m_modified = true;
    emit textChanged();
    emit modifiedChanged();
}

void PrimoDocument::insertText(int position, const QString& t)
{
    if (position < 0) position = 0;
    if (position > m_text.size()) position = m_text.size();
    m_text.insert(position, t);
    invalidateCache();
    m_modified = true;
    emit textChanged();
    emit modifiedChanged();
}

void PrimoDocument::removeText(int position, int length)
{
    if (position < 0 || length <= 0 || position >= m_text.size()) return;
    m_text.remove(position, length);
    invalidateCache();
    m_modified = true;
    emit textChanged();
    emit modifiedChanged();
}

int PrimoDocument::length() const { return m_text.size(); }

int PrimoDocument::lineStartOffset(int line) const
{
    rebuildOffsets();
    if (line < 0 || line >= m_lineOffsets.size()) return -1;
    return m_lineOffsets.at(line);
}

QStringList PrimoDocument::visibleLines(int firstLine, int count) const
{
    rebuildOffsets();
    if (firstLine < 0) firstLine = 0;
    if (count <= 0) return {};
    int end = qMin(firstLine + count, m_lineOffsets.size());
    QStringList out;
    out.reserve(end - firstLine);
    for (int i = firstLine; i < end; ++i) out.append(lineAt(i));
    return out;
}

bool PrimoDocument::loadFromFile(const QString& path, QString* error)
{
    FileLoadResult res = FileService::load(path);
    if (!res.ok) {
        if (error) *error = res.error;
        return false;
    }
    m_filePath = path;
    m_text = res.text;
    m_modified = false;
    invalidateCache();
    emit filePathChanged();
    emit textChanged();
    emit modifiedChanged();
    return true;
}

bool PrimoDocument::saveToFile(const QString& path, QString* error) const
{
    QString err;
    bool ok = FileService::save(path, m_text, QStringLiteral("UTF-8"), &err);
    if (!ok && error) *error = err;
    return ok;
}
