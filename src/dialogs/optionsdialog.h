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

#include "dialogsettings.h"

#include <QDialog>

class QComboBox;
class QCheckBox;
class QLineEdit;
class QPushButton;
class QSpinBox;
class QTabWidget;
class QTableWidget;

class ThemePreviewWidget;

class OptionsDialog : public QDialog
{
    Q_OBJECT

public:
    explicit OptionsDialog(QWidget* parent = nullptr);
    ~OptionsDialog() override;

signals:
    void themeChanged();

private slots:
    void accept() override;
    void reject() override;

private:
    void setupUI();
    QWidget* createScrollContainer(QWidget* content);
    void setupGeneralTab();
    void setupAppearanceTab();
    void setupEditorTab();
    void setupPanelsTab();
    void setupLspTab();
    void loadSettings();
    void saveSettings();
    void updateThemePreview();

    QTabWidget* tabWidget = nullptr;

    QComboBox* startupComboBox = nullptr;
    QComboBox* themeComboBox = nullptr;
    QComboBox* defaultFormatComboBox = nullptr;
    QComboBox* closeButtonComboBox = nullptr;
    QComboBox* defaultEncodingComboBox = nullptr;
    QComboBox* fontFaceComboBox = nullptr;
    QComboBox* tabDisplayComboBox = nullptr;
    QComboBox* cursorStyleComboBox = nullptr;
    QComboBox* tabBarPositionComboBox = nullptr;
    QComboBox* terminalPanelPositionComboBox = nullptr;
    QSpinBox* terminalPanelMinWidthSpin = nullptr;
    QComboBox* terminalFontFaceComboBox = nullptr;
    QLineEdit* terminalFontSizeBox = nullptr;
    QSpinBox* tabWidthSpin = nullptr;
    QSpinBox* verticalEdgeColumnSpin = nullptr;
    QLineEdit* fontSizeBox = nullptr;
    QComboBox* uiFontFaceComboBox = nullptr;
    QLineEdit* uiFontSizeBox = nullptr;
    QCheckBox* showLineNumbersCheckBox = nullptr;
    QCheckBox* scrollPastContentCheckBox = nullptr;
    QCheckBox* wordWrapCheckBox = nullptr;
    QCheckBox* codeCollapsingCheckBox = nullptr;
    QCheckBox* showWhitespaceCheckBox = nullptr;
    QCheckBox* cursorBlinkingCheckBox = nullptr;
    QCheckBox* highlightCurrentLineCheckBox = nullptr;
    QCheckBox* verticalEdgeCheckBox = nullptr;
    QCheckBox* autoCompletionCheckBox = nullptr;
    QSpinBox* autoCompletionThresholdSpin = nullptr;
    QCheckBox* autoCompletionCaseSensitiveCheckBox = nullptr;
    QCheckBox* autoCloseBracketsCheckBox = nullptr;
    QCheckBox* snippetsCheckBox = nullptr;
    QCheckBox* predictiveSnippetsCheckBox = nullptr;
    QCheckBox* autoSaveCheckBox = nullptr;
    QSpinBox* autoSaveIntervalSpin = nullptr;
    QCheckBox* showHiddenFilesCheckBox = nullptr;
    QCheckBox* useGitIgnoreCheckBox = nullptr;
    QComboBox* projectPanelPositionComboBox = nullptr;
    ThemePreviewWidget* themePreview = nullptr;
    QPushButton* accentColorButton = nullptr;
    QColor m_accentColor;
    DialogSettings m_geometrySettings;

    // LSP settings
    QCheckBox* lspEnabledCheckBox = nullptr;
    QCheckBox* lspShowErrorListCheckBox = nullptr;
    QSpinBox* lspCompletionTriggerSpin = nullptr;
    QTableWidget* lspServerTable = nullptr;
};

#endif // OPTIONSDIALOG_H
