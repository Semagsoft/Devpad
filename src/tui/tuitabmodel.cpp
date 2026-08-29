/*
 * Devpad - A C++/Qt6 code editor
 * Copyright (C) 2026 Semagsoft
 */

#include "tuitabmodel.h"

#include <QDir>
#include <QFileInfo>

void TuiTabModel::setCurrentIndex(int idx)
{
    if (m_buffers.isEmpty())
    {
        m_currentIndex = -1;
        return;
    }
    m_currentIndex = qBound(0, idx, m_buffers.size() - 1);
}

TuiBuffer* TuiTabModel::currentBuffer()
{
    return bufferAt(m_currentIndex);
}

const TuiBuffer* TuiTabModel::currentBuffer() const
{
    return bufferAt(m_currentIndex);
}

TuiBuffer* TuiTabModel::bufferAt(int idx)
{
    if (idx < 0 || idx >= m_buffers.size())
        return nullptr;
    return &m_buffers[idx];
}

const TuiBuffer* TuiTabModel::bufferAt(int idx) const
{
    if (idx < 0 || idx >= m_buffers.size())
        return nullptr;
    return &m_buffers[idx];
}

int TuiTabModel::addBuffer(const TuiBuffer& buf)
{
    // Deduplicate by canonical file path similar to TabManager::findEditorByFileName
    if (!buf.filePath().isEmpty())
    {
        int existing = findByFilePath(buf.filePath());
        if (existing != -1)
        {
            m_currentIndex = existing;
            return existing;
        }
    }
    m_buffers.append(buf);
    m_currentIndex = m_buffers.size() - 1;
    return m_currentIndex;
}

void TuiTabModel::closeBuffer(int idx)
{
    if (idx < 0 || idx >= m_buffers.size())
        return;
    m_buffers.removeAt(idx);
    if (m_buffers.isEmpty())
    {
        m_currentIndex = -1;
    }
    else if (m_currentIndex >= m_buffers.size())
    {
        m_currentIndex = m_buffers.size() - 1;
    }
    else if (m_currentIndex > idx)
    {
        m_currentIndex--;
    }
}

void TuiTabModel::closeCurrent()
{
    closeBuffer(m_currentIndex);
}

int TuiTabModel::findByFilePath(const QString& filePath) const
{
    if (filePath.isEmpty())
        return -1;
    QString canonical = QFileInfo(filePath).canonicalFilePath();
    if (canonical.isEmpty())
        canonical = QDir::cleanPath(QFileInfo(filePath).absoluteFilePath());
    for (int i = 0; i < m_buffers.size(); ++i)
    {
        QString p = m_buffers[i].filePath();
        if (p.isEmpty())
            continue;
        QString c2 = QFileInfo(p).canonicalFilePath();
        if (c2.isEmpty())
            c2 = QDir::cleanPath(QFileInfo(p).absoluteFilePath());
        if (QString::compare(canonical, c2, Qt::CaseInsensitive) == 0 || canonical == c2)
            return i;
        // Fallback exact string compare
        if (p == filePath)
            return i;
    }
    return -1;
}

bool TuiTabModel::isPinned(int idx) const
{
    const TuiBuffer* b = bufferAt(idx);
    if (!b || b->filePath().isEmpty())
        return false;
    return m_pinned.contains(b->filePath());
}

void TuiTabModel::setPinned(int idx, bool pinned)
{
    const TuiBuffer* b = bufferAt(idx);
    if (!b || b->filePath().isEmpty())
        return;
    if (pinned)
        m_pinned.insert(b->filePath());
    else
        m_pinned.remove(b->filePath());
}

QStringList TuiTabModel::allFilePaths() const
{
    QStringList out;
    for (const auto& b : m_buffers)
    {
        if (!b.filePath().isEmpty())
            out.append(b.filePath());
    }
    return out;
}
