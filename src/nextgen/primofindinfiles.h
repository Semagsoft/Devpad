/*
 * Devpad - A C++/Qt6 code editor
 * Copyright (C) 2026 Semagsoft
 *
 * PrimoFindInFiles: QML bridge for Find-in-Files Drawer.
 * Respects .gitignore (GitIgnore) + showHidden (SettingsManager::showHiddenFiles)
 * and useGitIgnore (SettingsManager::useGitIgnore) as TuiFileTree does.
 */

#ifndef PRIMOFINDINFILES_H
#define PRIMOFINDINFILES_H

#include <QObject>
#include <QVariantList>

struct FindInFilesResult;

class PrimoFindInFiles : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString lastPattern READ lastPattern NOTIFY lastPatternChanged)
    Q_PROPERTY(QString lastRoot READ lastRoot NOTIFY lastRootChanged)
    Q_PROPERTY(int resultCount READ resultCount NOTIFY resultCountChanged)

public:
    explicit PrimoFindInFiles(QObject* parent = nullptr);

    QString lastPattern() const { return m_lastPattern; }
    QString lastRoot() const { return m_lastRoot; }
    int resultCount() const { return m_results.size(); }

    Q_INVOKABLE QVariantList search(const QString& pattern, const QString& rootPath, const QString& fileGlob = QString(), bool caseSensitive = false, bool wholeWord = false, bool useRegex = false);
    Q_INVOKABLE void clear();
    Q_INVOKABLE QVariantMap resultAt(int idx) const;
    Q_INVOKABLE QString fileAt(int idx) const;
    Q_INVOKABLE int lineAt(int idx) const;

signals:
    void lastPatternChanged();
    void lastRootChanged();
    void resultCountChanged();
    void searchFinished(int count);
    void searchStarted();

private:
    QString m_lastPattern;
    QString m_lastRoot;
    QVariantList m_results; // each is QVariantMap {filePath, line, column, length, lineText}
};

#endif // PRIMOFINDINFILES_H
