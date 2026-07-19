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
#ifndef REPLACEDIALOG_H
#define REPLACEDIALOG_H

#include "dialogsettings.h"

#include <QDialog>
#include <QPointer>

#include <Qsci/qsciscintilla.h>

class QCloseEvent;
class QLineEdit;
class QPushButton;
class QCheckBox;

class ReplaceDialog : public QDialog
{
    Q_OBJECT

public:
    explicit ReplaceDialog(QWidget* parent = nullptr);
    ~ReplaceDialog() override;

    QString findText() const;
    QString replaceText() const;
    bool matchCase() const;
    bool matchWholeWord() const;
    bool useRegex() const;
    void setFindText(const QString& text);
    void setEditor(QsciScintilla* editor);

signals:
    void searchFinished(bool found);

private slots:
    void onFindTextChanged(const QString& text);
    void findNext();
    void replace();
    void replaceAll();

protected:
    void closeEvent(QCloseEvent* event) override;

private:
    void setupUI();
    void loadSettings();
    void saveSettings();

    QLineEdit* findLineEdit = nullptr;
    QLineEdit* replaceLineEdit = nullptr;
    QPushButton* findNextButton = nullptr;
    QPushButton* replaceButton = nullptr;
    QPushButton* replaceAllButton = nullptr;
    QPushButton* closeButton = nullptr;
    QCheckBox* matchCaseCheckBox = nullptr;
    QCheckBox* matchWholeWordCheckBox = nullptr;
    QCheckBox* useRegexCheckBox = nullptr;
    QPointer<QsciScintilla> editor;
    DialogSettings m_settings;
};

#endif // REPLACEDIALOG_H
