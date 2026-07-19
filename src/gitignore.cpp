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

    QRegularExpression regex = patternToRegex(pattern, anchored);
    if (!regex.isValid())
        return;

    if (dirOnly)
    {
        QString p = regex.pattern();
        p.replace(QRegularExpression("\\$$"), "(/.*)?$");
        regex = QRegularExpression(p);
    }

    m_patterns[dirPath].append({regex, negate, dirOnly, anchored});
}

QString GitIgnore::relativePath(const QString& filePath, const QString& dirPath) const
{
    QString absFile = QFileInfo(filePath).absoluteFilePath();
    QString absDir = QFileInfo(dirPath).absoluteFilePath();

    if (!absFile.startsWith(absDir + '/') && absFile != absDir)
        return {};

    if (absFile == absDir)
        return {};

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
            regex += ".*";
            i += 2;
            if (i < pattern.size() && pattern[i] == '/')
                i++;
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

    QString absPath = QFileInfo(filePath).absoluteFilePath();
    QString fileDir = isDir ? absPath : QFileInfo(filePath).absolutePath();

    if (!fileDir.startsWith(m_rootPath + '/') && fileDir != m_rootPath)
        return false;

    bool ignored = false;

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

        for (const Pattern& p : it.value())
        {
            QString matchTarget;
            if (p.dirOnly || p.anchored || !relPath.contains('/'))
            {
                matchTarget = relPath;
            }
            else
            {
                matchTarget = relPath.section('/', -1);
            }

            if (p.regex.match(matchTarget).hasMatch())
                ignored = !p.negate;
        }
    }

    return ignored;
}
