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
#include "replacedialog.h"

#include <QCheckBox>
#include <QCloseEvent>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QPushButton>
#include <QVBoxLayout>

ReplaceDialog::ReplaceDialog(QWidget* parent) : QDialog(parent), editor(nullptr), m_settings("Replace")
{
    setWindowTitle(tr("Replace"));
    setupUI();
    loadSettings();
}

ReplaceDialog::~ReplaceDialog() = default;

void ReplaceDialog::closeEvent(QCloseEvent* event)
{
    saveSettings();
    QDialog::closeEvent(event);
}

void ReplaceDialog::setupUI()
{
    QVBoxLayout* mainLayout = new QVBoxLayout(this);

    // Find text
    QHBoxLayout* findLayout = new QHBoxLayout();
    QLabel* findLabel = new QLabel(tr("Find:"), this);
    findLineEdit = new QLineEdit(this);
    findLayout->addWidget(findLabel);
    findLayout->addWidget(findLineEdit);
    mainLayout->addLayout(findLayout);

    // Replace text
    QHBoxLayout* replaceLayout = new QHBoxLayout();
    QLabel* replaceLabel = new QLabel(tr("Replace:"), this);
    replaceLineEdit = new QLineEdit(this);
    replaceLayout->addWidget(replaceLabel);
    replaceLayout->addWidget(replaceLineEdit);
    mainLayout->addLayout(replaceLayout);

    // Options
    QGroupBox* optionsGroup = new QGroupBox(tr("Options"), this);
    QVBoxLayout* optionsLayout = new QVBoxLayout(optionsGroup);
    matchCaseCheckBox = new QCheckBox(tr("Match case"), this);
    matchWholeWordCheckBox = new QCheckBox(tr("Match whole word"), this);
    useRegexCheckBox = new QCheckBox(tr("Use regular expressions"), this);
    optionsLayout->addWidget(matchCaseCheckBox);
    optionsLayout->addWidget(matchWholeWordCheckBox);
    optionsLayout->addWidget(useRegexCheckBox);
    mainLayout->addWidget(optionsGroup);

    // Buttons
    QHBoxLayout* buttonLayout = new QHBoxLayout();
    findNextButton = new QPushButton(tr("Find Next"), this);
    replaceButton = new QPushButton(tr("Replace"), this);
    replaceAllButton = new QPushButton(tr("Replace All"), this);
    closeButton = new QPushButton(tr("Close"), this);
    findNextButton->setEnabled(false);
    replaceButton->setEnabled(false);
    replaceAllButton->setEnabled(false);
    buttonLayout->addWidget(findNextButton);
    buttonLayout->addWidget(replaceButton);
    buttonLayout->addWidget(replaceAllButton);
    buttonLayout->addStretch();
    buttonLayout->addWidget(closeButton);
    mainLayout->addLayout(buttonLayout);

    connect(findLineEdit, &QLineEdit::textChanged, this, &ReplaceDialog::onFindTextChanged);
    connect(findNextButton, &QPushButton::clicked, this, &ReplaceDialog::findNext);
    connect(replaceButton, &QPushButton::clicked, this, &ReplaceDialog::replace);
    connect(replaceAllButton, &QPushButton::clicked, this, &ReplaceDialog::replaceAll);
    connect(closeButton, &QPushButton::clicked, this, &QDialog::close);

    findLineEdit->setFocus();
}

QString ReplaceDialog::findText() const
{
    return findLineEdit->text();
}

void ReplaceDialog::setFindText(const QString& text)
{
    findLineEdit->setText(text);
}

QString ReplaceDialog::replaceText() const
{
    return replaceLineEdit->text();
}

bool ReplaceDialog::matchCase() const
{
    return matchCaseCheckBox->isChecked();
}

bool ReplaceDialog::matchWholeWord() const
{
    return matchWholeWordCheckBox->isChecked();
}

bool ReplaceDialog::useRegex() const
{
    return useRegexCheckBox->isChecked();
}

void ReplaceDialog::setEditor(QsciScintilla* editor)
{
    this->editor = editor;
}

void ReplaceDialog::onFindTextChanged(const QString& text)
{
    bool enabled = !text.isEmpty();
    findNextButton->setEnabled(enabled);
    replaceButton->setEnabled(enabled);
    replaceAllButton->setEnabled(enabled);
}

void ReplaceDialog::findNext()
{
    if (!editor)
        return;
    bool found = editor->findFirst(findText(), useRegex(), matchCase(), matchWholeWord(), true);
    emit searchFinished(found);
}

void ReplaceDialog::replace()
{
    if (!editor)
        return;

    if (editor->hasSelectedText())
    {
        editor->replaceSelectedText(replaceText());
        editor->findFirst(findText(), useRegex(), matchCase(), matchWholeWord(), true);
        emit searchFinished(true);
        return;
    }

    bool found = editor->findFirst(findText(), useRegex(), matchCase(), matchWholeWord(), true);
    if (found)
    {
        editor->replaceSelectedText(replaceText());
    }
    emit searchFinished(found);
}

void ReplaceDialog::replaceAll()
{
    if (!editor)
        return;
    editor->beginUndoAction();
    editor->setCursorPosition(0, 0);
    int count = 0;
    int maxIterations = editor->length() + 1;
    while (editor->findFirst(findText(), useRegex(), matchCase(), matchWholeWord(), true))
    {
        editor->replaceSelectedText(replaceText());
        count++;
        if (--maxIterations <= 0)
        {
            QMessageBox::warning(this, tr("Replace All"), tr("Maximum replacement limit reached. Possible infinite loop detected."));
            break;
        }
    }
    editor->endUndoAction();
    QMessageBox::information(this, tr("Replace All"), tr("Replaced %1 occurrence(s).").arg(count));
}

void ReplaceDialog::loadSettings()
{
    matchCaseCheckBox->setChecked(m_settings.load("MatchCase", false).toBool());
    matchWholeWordCheckBox->setChecked(m_settings.load("MatchWholeWord", false).toBool());
    useRegexCheckBox->setChecked(m_settings.load("UseRegex", false).toBool());
}

void ReplaceDialog::saveSettings()
{
    m_settings.save("MatchCase", matchCaseCheckBox->isChecked());
    m_settings.save("MatchWholeWord", matchWholeWordCheckBox->isChecked());
    m_settings.save("UseRegex", useRegexCheckBox->isChecked());
}
