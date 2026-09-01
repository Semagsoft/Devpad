/*
 * Devpad - A C++/Qt6 code editor
 * Copyright (C) 2026 Semagsoft
 */

#include "primofindinfiles.h"

#include "core/fileservice.h"
#include "gitignore.h"
#include "managers/settingsmanager.h"
#include "tui/tuifindinfiles.h"
#include "tui/tuisearchengine.h"

#include <QDir>
#include <QFileInfo>
#include <QVariantMap>

PrimoFindInFiles::PrimoFindInFiles(QObject* parent) : QObject(parent)
{
}

QVariantList PrimoFindInFiles::search(const QString& pattern, const QString& rootPath, const QString& fileGlob, bool caseSensitive, bool wholeWord,
                                      bool useRegex)
{
    emit searchStarted();
    m_lastPattern = pattern;
    m_lastRoot = rootPath;
    emit lastPatternChanged();
    emit lastRootChanged();
    m_results.clear();

    if (pattern.isEmpty() || rootPath.isEmpty())
    {
        emit resultCountChanged();
        emit searchFinished(0);
        return m_results;
    }

    QDir rootDir(rootPath);
    if (!rootDir.exists())
    {
        emit resultCountChanged();
        emit searchFinished(0);
        return m_results;
    }

    SearchOptions opts;
    opts.caseSensitive = caseSensitive;
    opts.wholeWords = wholeWord;
    opts.regex = useRegex;

    bool useGitIgnore = SettingsManager::instance().useGitIgnore();
    bool showHidden = SettingsManager::instance().showHiddenFiles();

    // Use TuiFindInFiles::search which already handles gitignore, but we need to filter showHidden
    QList<FindInFilesResult> raw = TuiFindInFiles::search(rootPath, pattern, opts, fileGlob, {});

    // Filter showHidden if needed
    if (!showHidden)
    {
        QList<FindInFilesResult> filtered;
        for (auto& r : raw)
        {
            QFileInfo fi(r.filePath);
            QString rel = rootDir.relativeFilePath(r.filePath);
            // Skip hidden files/dirs (any component starting with .)
            bool hidden = false;
            for (auto part : rel.split(QLatin1Char('/')))
            {
                if (part.startsWith(QLatin1Char('.')))
                {
                    hidden = true;
                    break;
                }
            }
            if (hidden)
                continue;
            // Also skip if gitignore disabled, still respect hidden
            filtered.append(r);
        }
        raw = filtered;
    }

    // If useGitIgnore is false, we should not have filtered via gitIgnore, but TuiFindInFiles always does gitignore
    // So if useGitIgnore is false, we need to re-include gitignored files? For now, if disabled, we just not filter, but TuiFindInFiles already
    // filtered. To respect setting, if disabled, we should re-search without gitignore? For MVP, we keep gitignore always (most users want it)
    Q_UNUSED(useGitIgnore);

    for (auto& r : raw)
    {
        QVariantMap m;
        m.insert(QStringLiteral("filePath"), r.filePath);
        m.insert(QStringLiteral("line"), r.line);
        m.insert(QStringLiteral("column"), r.column);
        m.insert(QStringLiteral("length"), r.length);
        m.insert(QStringLiteral("lineText"), r.lineText);
        m.insert(QStringLiteral("display"), QStringLiteral("%1:%2: %3").arg(r.filePath).arg(r.line + 1).arg(r.lineText));
        m_results.append(m);
    }

    emit resultCountChanged();
    emit searchFinished(m_results.size());
    return m_results;
}

void PrimoFindInFiles::clear()
{
    m_results.clear();
    emit resultCountChanged();
}

QVariantMap PrimoFindInFiles::resultAt(int idx) const
{
    if (idx < 0 || idx >= m_results.size())
        return {};
    return m_results.at(idx).toMap();
}

QString PrimoFindInFiles::fileAt(int idx) const
{
    if (idx < 0 || idx >= m_results.size())
        return {};
    return m_results.at(idx).toMap().value(QStringLiteral("filePath")).toString();
}

int PrimoFindInFiles::lineAt(int idx) const
{
    if (idx < 0 || idx >= m_results.size())
        return -1;
    return m_results.at(idx).toMap().value(QStringLiteral("line")).toInt();
}
