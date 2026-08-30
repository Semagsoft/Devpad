/*
 * Devpad - A C++/Qt6 code editor
 * Copyright (C) 2026 Semagsoft
 */

#include "tuifindinfiles.h"

#include "core/fileservice.h"
#include "gitignore.h"

#include <QDir>
#include <QDirIterator>
#include <QFileInfo>
#include <QRegularExpression>

QList<FindInFilesResult> TuiFindInFiles::search(const QString& rootPath, const QString& pattern, const SearchOptions& opts,
                                                const QString& fileGlob, const QStringList& excludeDirs)
{
    QList<FindInFilesResult> results;
    if (pattern.isEmpty() || rootPath.isEmpty())
        return results;

    QDir rootDir(rootPath);
    if (!rootDir.exists())
        return results;

    // Prepare glob list
    QStringList globs = parseGlob(fileGlob);

    // Prepare exclude set
    QStringList excludes = excludeDirs;
    if (excludes.isEmpty())
        excludes << QStringLiteral(".git") << QStringLiteral("node_modules") << QStringLiteral("__pycache__") << QStringLiteral("build") << QStringLiteral("_build");

    GitIgnore gitIgnore;
    gitIgnore.setRootPath(rootPath);
    gitIgnore.scanDirectory(rootPath);

    QDirIterator it(rootPath, QDir::Files | QDir::NoDotAndDotDot, QDirIterator::Subdirectories);
    // QDirIterator with Subdirectories will iterate recursively; we need to filter exclude dirs manually
    while (it.hasNext())
    {
        QString filePath = it.next();
        QFileInfo fi(filePath);
        // Exclude dirs check
        QString rel = rootDir.relativeFilePath(filePath);
        bool excluded = false;
        for (const QString& ex : excludes)
        {
            if (rel.startsWith(ex + QLatin1Char('/')) || rel == ex)
            {
                excluded = true;
                break;
            }
        }
        if (excluded)
        {
            // Skip files under excluded dir - QDirIterator doesn't have easy skip, but we can continue
            // For performance, we rely on iterator to still walk but we skip results
            continue;
        }
        if (gitIgnore.isIgnored(filePath, false))
            continue;

        if (!globs.isEmpty())
        {
            bool matched = false;
            for (const QString& g : globs)
            {
                if (matchesGlob(fi.fileName(), g))
                {
                    matched = true;
                    break;
                }
            }
            if (!matched)
                continue;
        }

        // Skip large files
        if (fi.size() > FileService::MaxFileSize)
            continue;
        if (fi.size() > 10 * 1024 * 1024) // 10MB cap for find-in-files
            continue;

        FileLoadResult res = FileService::load(filePath);
        if (!res.ok)
            continue;

        QStringList lines = res.text.split(QLatin1Char('\n'));
        // Use TuiSearchEngine::findAll per file
        QList<SearchResult> matches = TuiSearchEngine::findAll(lines, pattern, opts);
        for (const SearchResult& m : matches)
        {
            FindInFilesResult r;
            r.filePath = filePath;
            r.line = m.line;
            r.column = m.column;
            r.length = m.length;
            r.lineText = lines[m.line].trimmed();
            if (r.lineText.size() > 200)
                r.lineText = r.lineText.left(200) + QStringLiteral("...");
            results.append(r);
            if (results.size() >= 1000)
                return results; // cap
        }
    }
    return results;
}

bool TuiFindInFiles::matchesGlob(const QString& fileName, const QString& glob)
{
    // Convert glob to regex: * -> .*, ? -> ., escape others
    QString pattern = QRegularExpression::escape(glob);
    pattern.replace(QStringLiteral("\\*"), QStringLiteral(".*"));
    pattern.replace(QStringLiteral("\\?"), QStringLiteral("."));
    QRegularExpression re(QStringLiteral("^") + pattern + QStringLiteral("$"), QRegularExpression::CaseInsensitiveOption);
    return re.match(fileName).hasMatch();
}

QStringList TuiFindInFiles::parseGlob(const QString& glob)
{
    if (glob.trimmed().isEmpty())
        return {};
    // Split by space or comma or semicolon
    QStringList parts = glob.split(QRegularExpression(QStringLiteral("[,;\\s]+")), Qt::SkipEmptyParts);
    QStringList out;
    for (QString p : parts)
    {
        p = p.trimmed();
        if (!p.isEmpty())
            out << p;
    }
    return out;
}
