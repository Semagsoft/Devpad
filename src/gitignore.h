#ifndef GITIGNORE_H
#define GITIGNORE_H

#include <QHash>
#include <QList>
#include <QRegularExpression>
#include <QString>

class GitIgnore
{
public:
    GitIgnore();
    explicit GitIgnore(const QString& rootPath);

    void setRootPath(const QString& rootPath);
    void clear();
    [[nodiscard]] bool isIgnored(const QString& filePath, bool isDir) const;
    [[nodiscard]] bool isEmpty() const;
    void scanDirectory(const QString& dirPath);

private:
    struct Pattern
    {
        QRegularExpression regex;
        bool negate = false;
        bool dirOnly = false;
        bool anchored = false;
    };

    void parseFile(const QString& filePath, const QString& dirPath);
    void addPattern(const QString& line, const QString& dirPath);
    [[nodiscard]] QString relativePath(const QString& filePath, const QString& dirPath) const;
    QRegularExpression patternToRegex(const QString& pattern, bool anchored) const;

    QString m_rootPath;
    QHash<QString, QList<Pattern>> m_patterns;
};

#endif
