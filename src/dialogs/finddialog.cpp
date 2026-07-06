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
#include "finddialog.h"
#include <QCloseEvent>
#include <QLineEdit>
#include <QPushButton>
#include <QCheckBox>
#include <QLabel>
#include <QGroupBox>
#include <QVBoxLayout>
#include <QHBoxLayout>

FindDialog::FindDialog(QWidget *parent) : QDialog(parent), editor(nullptr), m_settings("Find") {
    setWindowTitle(tr("Find"));
    setupUI();
    loadSettings();
}

FindDialog::~FindDialog() = default;

void FindDialog::closeEvent(QCloseEvent *event) {
    saveSettings();
    QDialog::closeEvent(event);
}

void FindDialog::setupUI() {
    QVBoxLayout *mainLayout = new QVBoxLayout(this);

    // Search text
    QHBoxLayout *searchLayout = new QHBoxLayout();
    QLabel *findLabel = new QLabel(tr("Find:"), this);
    searchLineEdit = new QLineEdit(this);
    searchLayout->addWidget(findLabel);
    searchLayout->addWidget(searchLineEdit);
    mainLayout->addLayout(searchLayout);

    // Options
    QGroupBox *optionsGroup = new QGroupBox(tr("Options"), this);
    QVBoxLayout *optionsLayout = new QVBoxLayout(optionsGroup);
    matchCaseCheckBox = new QCheckBox(tr("Match case"), this);
    matchWholeWordCheckBox = new QCheckBox(tr("Match whole word"), this);
    searchUpCheckBox = new QCheckBox(tr("Search up"), this);
    useRegexCheckBox = new QCheckBox(tr("Use regular expressions"), this);
    optionsLayout->addWidget(matchCaseCheckBox);
    optionsLayout->addWidget(matchWholeWordCheckBox);
    optionsLayout->addWidget(searchUpCheckBox);
    optionsLayout->addWidget(useRegexCheckBox);
    mainLayout->addWidget(optionsGroup);

    // Buttons
    QHBoxLayout *buttonLayout = new QHBoxLayout();
    findNextButton = new QPushButton(tr("Find Next"), this);
    findPrevButton = new QPushButton(tr("Find Previous"), this);
    closeButton = new QPushButton(tr("Close"), this);
    findNextButton->setEnabled(false);
    findPrevButton->setEnabled(false);
    buttonLayout->addWidget(findNextButton);
    buttonLayout->addWidget(findPrevButton);
    buttonLayout->addStretch();
    buttonLayout->addWidget(closeButton);
    mainLayout->addLayout(buttonLayout);

    connect(searchLineEdit, &QLineEdit::textChanged, this, &FindDialog::onTextChanged);
    connect(findNextButton, &QPushButton::clicked, this, &FindDialog::findNext);
    connect(findPrevButton, &QPushButton::clicked, this, &FindDialog::findPrevious);
    connect(closeButton, &QPushButton::clicked, this, &QDialog::close);

    searchLineEdit->setFocus();
}

QString FindDialog::searchText() const {
    return searchLineEdit->text();
}

void FindDialog::setSearchText(const QString &text) {
    searchLineEdit->setText(text);
}

bool FindDialog::matchCase() const {
    return matchCaseCheckBox->isChecked();
}

bool FindDialog::matchWholeWord() const {
    return matchWholeWordCheckBox->isChecked();
}

bool FindDialog::searchUp() const {
    return searchUpCheckBox->isChecked();
}

bool FindDialog::useRegex() const {
    return useRegexCheckBox->isChecked();
}

void FindDialog::setEditor(QsciScintilla *editor) {
    this->editor = editor;
}

void FindDialog::onTextChanged(const QString &text) {
    bool enabled = !text.isEmpty();
    findNextButton->setEnabled(enabled);
    findPrevButton->setEnabled(enabled);
}

void FindDialog::findNext() {
    if (!editor) return;
    bool found = editor->findFirst(searchText(), useRegex(), matchCase(), matchWholeWord(), true, !searchUp());
    emit searchFinished(found);
}

void FindDialog::findPrevious() {
    if (!editor) return;
    bool found = editor->findFirst(searchText(), useRegex(), matchCase(), matchWholeWord(), true, searchUp());
    emit searchFinished(found);
}

void FindDialog::loadSettings() {
    matchCaseCheckBox->setChecked(m_settings.load("MatchCase", false).toBool());
    matchWholeWordCheckBox->setChecked(m_settings.load("MatchWholeWord", false).toBool());
    searchUpCheckBox->setChecked(m_settings.load("SearchUp", false).toBool());
    useRegexCheckBox->setChecked(m_settings.load("UseRegex", false).toBool());
}

void FindDialog::saveSettings() {
    m_settings.save("MatchCase", matchCaseCheckBox->isChecked());
    m_settings.save("MatchWholeWord", matchWholeWordCheckBox->isChecked());
    m_settings.save("SearchUp", searchUpCheckBox->isChecked());
    m_settings.save("UseRegex", useRegexCheckBox->isChecked());
}
