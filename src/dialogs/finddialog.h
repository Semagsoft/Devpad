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
#ifndef FINDDIALOG_H
#define FINDDIALOG_H

#include <QDialog>
#include <QCloseEvent>
#include <QLineEdit>
#include <QPushButton>
#include <QCheckBox>
#include <QGroupBox>
#include <QPointer>
#include <Qsci/qsciscintilla.h>
#include "dialogsettings.h"

class FindDialog : public QDialog {
    Q_OBJECT

public:
    explicit FindDialog(QWidget *parent = nullptr);
    ~FindDialog() override;

    QString searchText() const;
    bool matchCase() const;
    bool matchWholeWord() const;
    bool searchUp() const;
    bool useRegex() const;
    void setSearchText(const QString &text);
    void setEditor(QsciScintilla *editor);

signals:
    void searchFinished(bool found);

private slots:
    void onTextChanged(const QString &text);
    void findNext();
    void findPrevious();

protected:
    void closeEvent(QCloseEvent *event) override;

private:
    void setupUI();
    void loadSettings();
    void saveSettings();

    QLineEdit *searchLineEdit;
    QPushButton *findNextButton;
    QPushButton *findPrevButton;
    QPushButton *closeButton;
    QCheckBox *matchCaseCheckBox;
    QCheckBox *matchWholeWordCheckBox;
    QCheckBox *searchUpCheckBox;
    QCheckBox *useRegexCheckBox;
    QPointer<QsciScintilla> editor;
    DialogSettings m_settings;
};

#endif // FINDDIALOG_H
