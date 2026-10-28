#include "gitignore.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QTextStream>

GitIgnore::GitIgnore() = default;

GitIgnore::GitIgnore(const QString& rootPath)
{
    setRootPath(rootPath);
}

void GitIgnore::setRootPath(const QString& rootPath)
{
    clear();
    m_rootPath = QFileInfo(rootPath).absoluteFilePath();

    QString gitIgnorePath = m_rootPath + "/.gitignore";
    if (QFile::exists(gitIgnorePath))
        parseFile(gitIgnorePath, m_rootPath);
}

void GitIgnore::clear()
{
    m_rootPath.clear();
    m_patterns.clear();
}

bool GitIgnore::isEmpty() const
{
    return m_patterns.isEmpty();
}

void GitIgnore::scanDirectory(const QString& dirPath)
{
    if (m_rootPath.isEmpty())
        return;

    QString absDir = QFileInfo(dirPath).absoluteFilePath();
    if (!absDir.startsWith(m_rootPath))
        return;

    if (m_patterns.contains(absDir))
        return;

    QString gitIgnorePath = absDir + "/.gitignore";
    if (QFile::exists(gitIgnorePath))
        parseFile(gitIgnorePath, absDir);
}

void GitIgnore::parseFile(const QString& filePath, const QString& dirPath)
{
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
        return;

    QTextStream in(&file);
    while (!in.atEnd())
    {
        QString line = in.readLine().trimmed();
        addPattern(line, dirPath);
    }
}

void GitIgnore::addPattern(const QString& line, const QString& dirPath)
{
    if (line.isEmpty() || line.startsWith('#'))
        return;

    QString pattern = line;
    bool negate = false;
    if (pattern.startsWith('!'))
    {
        negate = true;
        pattern = pattern.mid(1);
    }

    if (pattern.isEmpty())
        return;

    bool dirOnly = pattern.endsWith('/');
    if (dirOnly)
        pattern.chop(1);

    bool anchored = pattern.startsWith('/');
    if (anchored)
        pattern = pattern.mid(1);

    bool hasSlash = pattern.contains('/');

    QRegularExpression regex = patternToRegex(pattern, anchored);
    if (!regex.isValid())
        return;

    if (dirOnly)
    {
        QString p = regex.pattern();
        p.replace(QRegularExpression("\\$$"), "(/.*)?$");
        regex = QRegularExpression(p);
    }

    m_patterns[dirPath].append({regex, negate, dirOnly, anchored, hasSlash});
}

QString GitIgnore::relativePath(const QString& filePath, const QString& dirPath) const
{
    QString absFile = QFileInfo(filePath).absoluteFilePath();
    QString absDir = QFileInfo(dirPath).absoluteFilePath();

    if (!absFile.startsWith(absDir + '/') && absFile != absDir)
        return QString();

    if (absFile == absDir)
        return QString();

    QString rel = absFile.mid(absDir.length() + 1);
    return rel;
}

QRegularExpression GitIgnore::patternToRegex(const QString& pattern, bool anchored) const
{
    QString regex;
    int i = 0;

    if (anchored)
        regex += '^';

    while (i < pattern.size())
    {
        QChar c = pattern[i];

        if (c == '*' && i + 1 < pattern.size() && pattern[i + 1] == '*')
        {
            i += 2;
            bool followedBySlash = (i < pattern.size() && pattern[i] == '/');
            if (followedBySlash)
                i++;

            if (i >= pattern.size())
            {
                // Bare or trailing '**': matches everything below this point.
                regex += ".*";
            }
            else if (followedBySlash)
            {
                // '**/' is a directory-boundary operator: '**/foo' matches
                // 'foo' and 'a/b/foo' (a component named foo), but NOT
                // 'myfoo' — a plain '.*foo' would over-match suffixes.
                regex += "(?:^|.*/)";
            }
            else
            {
                // '**' not followed by '/' (e.g. 'a**b'): crosses separators.
                regex += ".*";
            }
        }
        else if (c == '*')
        {
            regex += "[^/]*";
            i++;
        }
        else if (c == '?')
        {
            regex += "[^/]";
            i++;
        }
        else if (c == '.')
        {
            regex += "\\.";
            i++;
        }
        else if (c == '[')
        {
            int end = pattern.indexOf(']', i + 1);
            if (end == -1)
            {
                regex += "\\[";
                i++;
            }
            else
            {
                QString cls = pattern.mid(i, end - i + 1);
                regex += cls;
                i = end + 1;
            }
        }
        else if (c == '\\')
        {
            regex += "\\\\";
            i++;
        }
        else if (c == '+' || c == '^' || c == '$' || c == '{' || c == '}' || c == '(' || c == ')' || c == '|')
        {
            regex += '\\';
            regex += c;
            i++;
        }
        else
        {
            regex += c;
            i++;
        }
    }

    regex += '$';
    return QRegularExpression(regex);
}

bool GitIgnore::isIgnored(const QString& filePath, bool isDir) const
{
    if (m_patterns.isEmpty())
        return false;

    QFileInfo fi(filePath);
    QString absPath = fi.absoluteFilePath();
    QString fileDir = fi.isDir() ? absPath : fi.absolutePath();

    if (!fileDir.startsWith(m_rootPath + '/') && fileDir != m_rootPath)
        return false;

    bool ignored = false;
    // Set when a directory component above the target is excluded: git
    // semantics say nothing beneath an excluded directory can be re-included
    // by negation patterns from deeper .gitignore files.
    bool insideExcludedDir = false;

    QStringList dirs;
    QString current = fileDir;
    while (current.startsWith(m_rootPath + '/') || current == m_rootPath)
    {
        dirs.prepend(current);
        if (current == m_rootPath)
            break;
        int slash = current.lastIndexOf('/');
        if (slash <= 0)
            break;
        current = current.left(slash);
    }

    for (const QString& dir : dirs)
    {
        auto it = m_patterns.find(dir);
        if (it == m_patterns.end())
            continue;

        QString relPath = relativePath(absPath, dir);
        if (relPath.isEmpty())
            continue;

        if (insideExcludedDir)
        {
            ignored = true;
            break;
        }

        bool dirExcludedHere = false;
        const bool hasDirBelow = relPath.contains('/');

        for (const Pattern& p : it.value())
        {
            if (p.dirOnly && !isDir && !hasDirBelow)
                continue; // a dir-only pattern never matches a bare file

            // 1) Match against the target itself.
            if (!(p.dirOnly && !isDir))
            {
                QString matchTarget;
                if (!p.dirOnly && !p.anchored && !p.hasSlash && hasDirBelow)
                    matchTarget = relPath.section('/', -1);
                else
                    matchTarget = relPath;

                if (p.regex.match(matchTarget).hasMatch())
                    ignored = !p.negate;
            }

            // 2) Match against the leading directory component: a pattern
            // that excludes a directory also excludes everything beneath it.
            if (hasDirBelow && p.regex.match(relPath.section('/', 0, 0)).hasMatch())
            {
                dirExcludedHere = !p.negate;
            }
        }

        if (dirExcludedHere)
        {
            insideExcludedDir = true;
            ignored = true;
        }
    }

    return ignored;
}
