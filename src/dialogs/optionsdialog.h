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
#ifndef OPTIONSDIALOG_H
#define OPTIONSDIALOG_H

#include <QDialog>
#include <QComboBox>
#include <QCheckBox>
#include <QLineEdit>
#include <QPushButton>
#include <QSpinBox>
#include <QTabWidget>
#include <QScrollArea>
#include <QGroupBox>

class ThemePreviewWidget;

class OptionsDialog : public QDialog {
    Q_OBJECT

public:
    explicit OptionsDialog(QWidget *parent = nullptr);
    ~OptionsDialog() override;

signals:
    void themeChanged();

private slots:
    void accept() override;

private:
    void setupUI();
    QWidget* createScrollContainer(QWidget *content);
    void loadSettings();
    void saveSettings();
    void updateThemePreview();

    QTabWidget *tabWidget;

    QComboBox *startupComboBox;
    QComboBox *themeComboBox;
    QComboBox *defaultFormatComboBox;
    QComboBox *closeButtonComboBox;
    QComboBox *defaultEncodingComboBox;
    QComboBox *fontFaceComboBox;
    QComboBox *tabDisplayComboBox;
    QComboBox *cursorStyleComboBox;
    QComboBox *tabBarPositionComboBox;
    QComboBox *terminalPanelPositionComboBox;
    QSpinBox *terminalPanelMinWidthSpin;
    QComboBox *terminalFontFaceComboBox;
    QLineEdit *terminalFontSizeBox;
    QSpinBox *tabWidthSpin;
    QSpinBox *verticalEdgeColumnSpin;
    QLineEdit *fontSizeBox;
    QComboBox *uiFontFaceComboBox;
    QLineEdit *uiFontSizeBox;
    QCheckBox *showLineNumbersCheckBox;
    QCheckBox *scrollPastContentCheckBox;
    QCheckBox *wordWrapCheckBox;
    QCheckBox *codeCollapsingCheckBox;
    QCheckBox *showWhitespaceCheckBox;
    QCheckBox *cursorBlinkingCheckBox;
    QCheckBox *highlightCurrentLineCheckBox;
    QCheckBox *verticalEdgeCheckBox;
    QCheckBox *autoCompletionCheckBox;
    QSpinBox *autoCompletionThresholdSpin;
    QCheckBox *autoCompletionCaseSensitiveCheckBox;
    QCheckBox *autoCloseBracketsCheckBox;
    QCheckBox *snippetsCheckBox;
    QCheckBox *predictiveSnippetsCheckBox;
    QCheckBox *autoSaveCheckBox;
    QSpinBox *autoSaveIntervalSpin;
    QCheckBox *showHiddenFilesCheckBox;
    QComboBox *projectPanelPositionComboBox;
    ThemePreviewWidget *themePreview;
    QPushButton *accentColorButton;
    QColor m_accentColor;
};

#endif // OPTIONSDIALOG_H
