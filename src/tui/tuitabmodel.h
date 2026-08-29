/*
 * Devpad - A C++/Qt6 code editor
 * Copyright (C) 2026 Semagsoft
 */

#ifndef TUITABMODEL_H
#define TUITABMODEL_H

#include "tuibuffer.h"

#include <QList>
#include <QSet>
#include <QString>

class TuiTabModel
{
public:
    TuiTabModel() = default;

    int count() const { return m_buffers.size(); }
    bool isEmpty() const { return m_buffers.isEmpty(); }

    int currentIndex() const { return m_currentIndex; }
    void setCurrentIndex(int idx);

    TuiBuffer* currentBuffer();
    const TuiBuffer* currentBuffer() const;

    TuiBuffer* bufferAt(int idx);
    const TuiBuffer* bufferAt(int idx) const;

    int addBuffer(const TuiBuffer& buf);
    void closeBuffer(int idx);
    void closeCurrent();

    int findByFilePath(const QString& filePath) const;

    QSet<QString> pinnedFiles() const { return m_pinned; }
    void setPinnedFiles(const QSet<QString>& p) { m_pinned = p; }
    bool isPinned(int idx) const;
    void setPinned(int idx, bool pinned);

    QStringList allFilePaths() const;

private:
    QList<TuiBuffer> m_buffers;
    int m_currentIndex = -1;
    QSet<QString> m_pinned;
};

#endif // TUITABMODEL_H
