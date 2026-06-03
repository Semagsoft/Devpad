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
#include "findinfilesdialog.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QFileDialog>
#include <QDir>
#include <QDirIterator>
#include <QFileInfo>
#include <QRegularExpression>
#include <QTextStream>

#include <QHeaderView>
#include <QKeyEvent>

FindInFilesWorker::FindInFilesWorker(const QString &searchText, const QString &rootDir,
                                     const QString &pattern, const QStringList &excludeDirs,
                                     bool matchCase, bool wholeWord, bool useRegex)
    : m_searchText(searchText), m_rootDir(rootDir), m_pattern(pattern),
      m_excludeDirs(excludeDirs), m_matchCase(matchCase),
      m_wholeWord(wholeWord), m_useRegex(useRegex) {
}

void FindInFilesWorker::run() {
    m_totalMatches = 0;

    if (m_searchText.isEmpty() || m_rootDir.isEmpty()) {
        emit searchFinished(0);
        return;
    }

    QStringList nameFilters;
    if (m_pattern.trimmed().isEmpty()) {
        nameFilters << "*";
    } else {
        QStringList parts = m_pattern.split(' ', Qt::SkipEmptyParts);
        for (const QString &p : parts) {
            QString trimmed = p.trimmed();
            if (!trimmed.isEmpty())
                nameFilters << trimmed;
        }
        if (nameFilters.isEmpty())
            nameFilters << "*";
    }

    QDirIterator it(m_rootDir, nameFilters, QDir::Files, QDirIterator::Subdirectories);
    while (it.hasNext()) {
        if (QThread::currentThread()->isInterruptionRequested()) {
            emit searchFinished(m_totalMatches);
            return;
        }

        QString filePath = it.next();

        bool excluded = false;
        for (const QString &exDir : m_excludeDirs) {
            if (filePath.contains(exDir)) {
                excluded = true;
                break;
            }
        }
        if (excluded)
            continue;

        searchFile(filePath);
        if (m_hasError) break;
    }

    emit searchFinished(m_totalMatches);
}

void FindInFilesWorker::searchFile(const QString &filePath) {
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly))
        return;

    if (file.size() > 10 * 1024 * 1024)
        return;

    QByteArray rawData = file.readAll();
    file.close();

    QStringConverter::Encoding encoding = QStringConverter::Utf8;
    if (rawData.startsWith(QByteArray("\xFF\xFE\x00\x00", 4))) {
        encoding = QStringConverter::Utf32LE;
        rawData.remove(0, 4);
    } else if (rawData.startsWith(QByteArray("\x00\x00\xFE\xFF", 4))) {
        encoding = QStringConverter::Utf32BE;
        rawData.remove(0, 4);
    } else if (rawData.startsWith(QByteArray("\xFF\xFE", 2))) {
        encoding = QStringConverter::Utf16LE;
        rawData.remove(0, 2);
    } else if (rawData.startsWith(QByteArray("\xFE\xFF", 2))) {
        encoding = QStringConverter::Utf16BE;
        rawData.remove(0, 2);
    } else if (rawData.startsWith(QByteArray("\xEF\xBB\xBF", 3))) {
        rawData.remove(0, 3);
    }

    QTextStream in(&rawData, QIODevice::ReadOnly);
    in.setEncoding(encoding);

    int lineNumber = 0;
    while (!in.atEnd()) {
        if (QThread::currentThread()->isInterruptionRequested()) {
            emit searchFinished(m_totalMatches);
            return;
        }

        QString line = in.readLine();
        lineNumber++;

        QString compareLine = (m_matchCase || m_useRegex) ? line : line.toLower();
        QString compareSearch = (m_matchCase || m_useRegex) ? m_searchText : m_searchText.toLower();

        bool matched = false;

        if (m_useRegex) {
            QRegularExpression::PatternOption opt = m_matchCase
                ? QRegularExpression::NoPatternOption
                : QRegularExpression::CaseInsensitiveOption;
            QRegularExpression re(compareSearch, opt);
            if (!re.isValid()) {
                emit searchError(tr("Invalid regex: %1").arg(re.errorString()));
                m_hasError = true;
                return;
            }
            matched = re.match(compareLine).hasMatch();
        } else if (m_wholeWord) {
            int idx = compareLine.indexOf(compareSearch);
            while (idx >= 0) {
                bool leftBoundary = (idx == 0) || !compareLine[idx - 1].isLetterOrNumber();
                int end = idx + compareSearch.length();
                bool rightBoundary = (end >= compareLine.length()) || !compareLine[end].isLetterOrNumber();
                if (leftBoundary && rightBoundary) {
                    matched = true;
                    break;
                }
                idx = compareLine.indexOf(compareSearch, idx + 1);
            }
        } else {
            matched = compareLine.contains(compareSearch);
        }

        if (matched) {
            m_totalMatches++;
            emit matchFound(filePath, lineNumber, line.trimmed());
        }
    }
}

FindInFilesDialog::FindInFilesDialog(const QString &defaultDir, QWidget *parent)
    : QDialog(parent), m_settings("FindInFiles") {
    setWindowTitle(tr("Find in Files"));
    setMinimumSize(700, 500);
    setupUI();
    loadSettings();

    if (!defaultDir.isEmpty())
        dirLineEdit->setText(defaultDir);

    connect(searchLineEdit, &QLineEdit::returnPressed, this, &FindInFilesDialog::onSearch);
}

FindInFilesDialog::~FindInFilesDialog() {
    onStop();
}

void FindInFilesDialog::setupUI() {
    auto *mainLayout = new QVBoxLayout(this);

    auto *searchLayout = new QHBoxLayout();
    searchLayout->addWidget(new QLabel(tr("Find:"), this));
    searchLineEdit = new QLineEdit(this);
    searchLineEdit->setPlaceholderText(tr("Enter search text..."));
    searchLayout->addWidget(searchLineEdit);
    mainLayout->addLayout(searchLayout);

    auto *dirLayout = new QHBoxLayout();
    dirLayout->addWidget(new QLabel(tr("In:"), this));
    dirLineEdit = new QLineEdit(this);
    dirLineEdit->setPlaceholderText(tr("Search directory..."));
    dirLayout->addWidget(dirLineEdit);
    browseButton = new QPushButton(tr("Browse..."), this);
    dirLayout->addWidget(browseButton);
    mainLayout->addLayout(dirLayout);

    auto *filterLayout = new QHBoxLayout();
    filterLayout->addWidget(new QLabel(tr("Filter:"), this));
    filterLineEdit = new QLineEdit(this);
    filterLineEdit->setPlaceholderText(tr("e.g. *.cpp *.h *.py (space-separated, * for all)"));
    filterLayout->addWidget(filterLineEdit);
    mainLayout->addLayout(filterLayout);

    auto *optionsLayout = new QHBoxLayout();
    matchCaseCheckBox = new QCheckBox(tr("Match case"), this);
    matchWholeWordCheckBox = new QCheckBox(tr("Whole word"), this);
    useRegexCheckBox = new QCheckBox(tr("Regex"), this);
    optionsLayout->addWidget(matchCaseCheckBox);
    optionsLayout->addWidget(matchWholeWordCheckBox);
    optionsLayout->addWidget(useRegexCheckBox);
    optionsLayout->addStretch();
    mainLayout->addLayout(optionsLayout);

    resultTree = new QTreeWidget(this);
    resultTree->setHeaderLabels({tr("File"), tr("Line"), tr("Match")});
    resultTree->setRootIsDecorated(false);
    resultTree->setAlternatingRowColors(true);
    resultTree->setSelectionMode(QAbstractItemView::SingleSelection);
    resultTree->setSortingEnabled(true);
    resultTree->header()->setStretchLastSection(true);
    resultTree->header()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
    resultTree->header()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
    resultTree->header()->setSectionResizeMode(2, QHeaderView::Stretch);
    mainLayout->addWidget(resultTree);

    auto *bottomLayout = new QHBoxLayout();
    statusLabel = new QLabel(this);
    bottomLayout->addWidget(statusLabel, 1);

    searchButton = new QPushButton(tr("Search"), this);
    stopButton = new QPushButton(tr("Stop"), this);
    stopButton->setEnabled(false);
    closeButton = new QPushButton(tr("Close"), this);

    bottomLayout->addWidget(searchButton);
    bottomLayout->addWidget(stopButton);
    bottomLayout->addWidget(closeButton);
    mainLayout->addLayout(bottomLayout);

    connect(browseButton, &QPushButton::clicked, this, &FindInFilesDialog::onBrowseDirectory);
    connect(searchButton, &QPushButton::clicked, this, &FindInFilesDialog::onSearch);
    connect(stopButton, &QPushButton::clicked, this, &FindInFilesDialog::onStop);
    connect(closeButton, &QPushButton::clicked, this, &QDialog::close);
    connect(resultTree, &QTreeWidget::itemDoubleClicked, this, &FindInFilesDialog::onResultDoubleClicked);

    searchLineEdit->setFocus();
}

void FindInFilesDialog::startSearch(const QString &searchText, const QString &directory) {
    searchLineEdit->setText(searchText);
    dirLineEdit->setText(directory);
    onSearch();
}

void FindInFilesDialog::onSearch() {
    onStop();

    resultTree->clear();
    m_matchCount = 0;
    statusLabel->setText(tr("Searching..."));

    QString searchText = searchLineEdit->text().trimmed();
    if (searchText.isEmpty())
        return;

    QString rootDir = dirLineEdit->text().trimmed();
    if (rootDir.isEmpty() || !QDir(rootDir).exists()) {
        statusLabel->setText(tr("Invalid directory."));
        return;
    }

    QStringList excludeDirs = {"/.git/", "/node_modules/", "/.svn/", "/__pycache__/", "/.hg/"};
    QString filter = filterLineEdit->text().trimmed();

    m_worker = new FindInFilesWorker(searchText, rootDir, filter, excludeDirs,
                                      matchCaseCheckBox->isChecked(),
                                      matchWholeWordCheckBox->isChecked(),
                                      useRegexCheckBox->isChecked());

    m_workerThread = new QThread(this);
    m_worker->moveToThread(m_workerThread);

    connect(m_workerThread, &QThread::started, m_worker, &FindInFilesWorker::run);
    connect(m_worker, &FindInFilesWorker::matchFound, this, &FindInFilesDialog::onMatchFound);
    connect(m_worker, &FindInFilesWorker::searchFinished, this, &FindInFilesDialog::onSearchFinished);
    connect(m_worker, &FindInFilesWorker::searchError, this, &FindInFilesDialog::onSearchError);

    searchButton->setEnabled(false);
    stopButton->setEnabled(true);

    m_searching = true;
    m_workerThread->start();
}

void FindInFilesDialog::onStop() {
    if (m_workerThread && m_workerThread->isRunning()) {
        m_workerThread->requestInterruption();
    }
    cleanupSearch();
}

void FindInFilesDialog::cleanupSearch() {
    if (m_worker) {
        m_worker->disconnect();
    }
    if (m_workerThread) {
        m_workerThread->disconnect();
        if (m_workerThread->isRunning()) {
            m_workerThread->quit();
            if (!m_workerThread->wait(3000)) {
                m_workerThread->terminate();
                m_workerThread->wait();
            }
        }
    }

    if (m_worker) {
        m_worker->deleteLater();
        m_worker = nullptr;
    }
    if (m_workerThread) {
        m_workerThread->deleteLater();
        m_workerThread = nullptr;
    }

    m_searching = false;
    searchButton->setEnabled(true);
    stopButton->setEnabled(false);
}

void FindInFilesDialog::onMatchFound(const QString &filePath, int lineNumber, const QString &lineText) {
    if (!m_searching) return;
    m_matchCount++;
    QString relativePath = QDir(dirLineEdit->text()).relativeFilePath(filePath);

    auto *item = new QTreeWidgetItem(resultTree);
    item->setText(0, relativePath);
    item->setText(1, QString::number(lineNumber));
    item->setText(2, lineText);
    item->setData(0, Qt::UserRole, filePath);
    item->setData(1, Qt::UserRole, lineNumber);

    if (m_matchCount == 1)
        resultTree->resizeColumnToContents(0);

    statusLabel->setText(tr("Found %1 match(es)...").arg(m_matchCount));
}

void FindInFilesDialog::onSearchFinished(int totalMatches) {
    Q_UNUSED(totalMatches)
    if (!m_searching) return;

    if (m_matchCount == 0)
        statusLabel->setText(tr("No results found."));
    else
        statusLabel->setText(tr("Found %1 match(es) in %2 file(s).")
            .arg(m_matchCount)
            .arg(resultTree->topLevelItemCount()));

    resultTree->resizeColumnToContents(0);
    resultTree->resizeColumnToContents(1);

    cleanupSearch();
}

void FindInFilesDialog::onSearchError(const QString &message) {
    statusLabel->setText(tr("Error: %1").arg(message));
}

void FindInFilesDialog::onBrowseDirectory() {
    QString dir = QFileDialog::getExistingDirectory(this, tr("Select Search Directory"),
                                                     dirLineEdit->text());
    if (!dir.isEmpty())
        dirLineEdit->setText(dir);
}

void FindInFilesDialog::onResultDoubleClicked(QTreeWidgetItem *item, int column) {
    Q_UNUSED(column)
    if (!item) return;

    QString filePath = item->data(0, Qt::UserRole).toString();
    int lineNumber = item->data(1, Qt::UserRole).toInt();
    emit navigateToResult(filePath, lineNumber);
}

void FindInFilesDialog::closeEvent(QCloseEvent *event) {
    onStop();
    saveSettings();
    QDialog::closeEvent(event);
}

void FindInFilesDialog::keyPressEvent(QKeyEvent *event) {
    if (event->key() == Qt::Key_Escape) {
        if (m_workerThread && m_workerThread->isRunning()) {
            onStop();
        } else {
            close();
        }
        event->accept();
        return;
    }
    QDialog::keyPressEvent(event);
}

void FindInFilesDialog::loadSettings() {
    matchCaseCheckBox->setChecked(m_settings.load("MatchCase", false).toBool());
    matchWholeWordCheckBox->setChecked(m_settings.load("MatchWholeWord", false).toBool());
    useRegexCheckBox->setChecked(m_settings.load("UseRegex", false).toBool());
    filterLineEdit->setText(m_settings.load("Filter", "*").toString());
}

void FindInFilesDialog::saveSettings() {
    m_settings.save("MatchCase", matchCaseCheckBox->isChecked());
    m_settings.save("MatchWholeWord", matchWholeWordCheckBox->isChecked());
    m_settings.save("UseRegex", useRegexCheckBox->isChecked());
    m_settings.save("Filter", filterLineEdit->text());
}
