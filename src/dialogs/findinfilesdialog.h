/*
 * Devpad - A C++/Qt6 code editor
 * Copyright (C) 2026 Semagsoft
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */
#ifndef FINDINFILESDIALOG_H
#define FINDINFILESDIALOG_H

#include <QDialog>
#include "dialogsettings.h"

class QLineEdit;
class QPushButton;
class QCheckBox;
class QTreeWidget;
class QTreeWidgetItem;
class QLabel;
class QThread;
class QKeyEvent;

class FindInFilesWorker : public QObject {
    Q_OBJECT

public:
    FindInFilesWorker(const QString &searchText, const QString &rootDir,
                      const QString &pattern, const QStringList &excludeDirs,
                      bool matchCase, bool wholeWord, bool useRegex);

public slots:
    void run();

signals:
    void matchFound(const QString &filePath, int lineNumber, const QString &lineText);
    void searchFinished(int totalMatches);
    void searchError(const QString &message);

private:
    void searchFile(const QString &filePath);

    QString m_searchText;
    QString m_rootDir;
    QString m_pattern;
    QStringList m_excludeDirs;
    bool m_matchCase;
    bool m_wholeWord;
    bool m_useRegex;
    bool m_hasError = false;
    int m_totalMatches = 0;
};

class FindInFilesDialog : public QDialog {
    Q_OBJECT

public:
    explicit FindInFilesDialog(const QString &defaultDir, QWidget *parent = nullptr);
    ~FindInFilesDialog() override;

    void startSearch(const QString &searchText, const QString &directory);

signals:
    void navigateToResult(const QString &filePath, int lineNumber);

private slots:
    void onSearch();
    void onStop();
    void onMatchFound(const QString &filePath, int lineNumber, const QString &lineText);
    void onSearchFinished(int totalMatches);
    void onSearchError(const QString &message);
    void onBrowseDirectory();
    void onResultDoubleClicked(QTreeWidgetItem *item, int column);

protected:
    void closeEvent(QCloseEvent *event) override;
    void keyPressEvent(QKeyEvent *event) override;

private:
    void setupUI();
    void loadSettings();
    void saveSettings();
    void cleanupSearch();

    QLineEdit *searchLineEdit;
    QLineEdit *dirLineEdit;
    QLineEdit *filterLineEdit;
    QPushButton *browseButton;
    QPushButton *searchButton;
    QPushButton *stopButton;
    QPushButton *closeButton;
    QCheckBox *matchCaseCheckBox;
    QCheckBox *matchWholeWordCheckBox;
    QCheckBox *useRegexCheckBox;
    QTreeWidget *resultTree;
    QLabel *statusLabel;

    QThread *m_workerThread = nullptr;
    FindInFilesWorker *m_worker = nullptr;
    DialogSettings m_settings;
    int m_matchCount = 0;
    bool m_searching = false;
};

#endif
