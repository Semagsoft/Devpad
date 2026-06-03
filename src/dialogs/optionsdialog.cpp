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
#include "optionsdialog.h"

#include "defaultsyntax.h"
#include "encodingutils.h"
#include "settingsmanager.h"
#include "theme.h"

#include <QFontDatabase>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QIntValidator>
#include <QMessageBox>
#include <QScrollArea>
#include <QVBoxLayout>

OptionsDialog::OptionsDialog(QWidget* parent) : QDialog(parent)
{
    setWindowTitle(tr("Options"));
    setModal(true);
    setMinimumSize(440, 400);
    setupUI();
    loadSettings();
}

OptionsDialog::~OptionsDialog() = default;

QWidget* OptionsDialog::createScrollContainer(QWidget* content)
{
    QScrollArea* scrollArea = new QScrollArea();
    scrollArea->setWidget(content);
    scrollArea->setWidgetResizable(true);
    scrollArea->setFrameShape(QScrollArea::NoFrame);
    return scrollArea;
}

void OptionsDialog::setupUI()
{
    QVBoxLayout* mainLayout = new QVBoxLayout(this);

    tabWidget = new QTabWidget(this);

    // === General Tab ===
    QWidget* generalContent = new QWidget();
    QVBoxLayout* generalMainLayout = new QVBoxLayout(generalContent);

    QGroupBox* startupGroup = new QGroupBox(tr("Startup"), generalContent);
    QFormLayout* startupLayout = new QFormLayout(startupGroup);
    startupComboBox = new QComboBox(startupGroup);
    startupComboBox->addItems({tr("New File"), tr("Restore Session"), tr("Open Last File"), tr("Empty")});
    startupLayout->addRow(tr("Startup mode:"), startupComboBox);
    generalMainLayout->addWidget(startupGroup);

    QGroupBox* defaultsGroup = new QGroupBox(tr("Defaults"), generalContent);
    QFormLayout* defaultsLayout = new QFormLayout(defaultsGroup);
    defaultFormatComboBox = new QComboBox(defaultsGroup);
    const QHash<QString, QString> displayNames = {
        {"cpp", tr("C++")},
        {"c", tr("C")},
        {"csharp", tr("C#")},
        {"java", tr("Java")},
        {"python", tr("Python")},
        {"javascript", tr("JavaScript")},
        {"typescript", tr("TypeScript")},
        {"html", tr("HTML")},
        {"css", tr("CSS")},
        {"xml", tr("XML")},
        {"sql", tr("SQL")},
        {"rust", tr("Rust")},
        {"go", tr("Go")},
        {"markdown", tr("Markdown")},
        {"bash", tr("Bash")},
        {"cmake", tr("CMake")},
    };
    for (const QString& lang : defaultSyntaxLanguages())
    {
        defaultFormatComboBox->addItem(displayNames.value(lang, lang));
    }
    defaultsLayout->addRow(tr("Default format:"), defaultFormatComboBox);
    defaultEncodingComboBox = new QComboBox(defaultsGroup);
    for (const auto& enc : supportedEncodings())
    {
        defaultEncodingComboBox->addItem(enc.displayName);
    }
    defaultsLayout->addRow(tr("Default encoding:"), defaultEncodingComboBox);
    generalMainLayout->addWidget(defaultsGroup);
    generalMainLayout->addStretch();

    tabWidget->addTab(createScrollContainer(generalContent), tr("General"));

    // === Appearance Tab ===
    QWidget* appearanceContent = new QWidget();
    QVBoxLayout* appearanceMainLayout = new QVBoxLayout(appearanceContent);

    QGroupBox* themeGroup = new QGroupBox(tr("Theme"), appearanceContent);
    QFormLayout* themeLayout = new QFormLayout(themeGroup);
    themeComboBox = new QComboBox(themeGroup);
    for (int i = 0; i <= static_cast<int>(ThemeId::GruvboxDark); ++i)
    {
        themeComboBox->addItem(themeDisplayName(static_cast<ThemeId>(i)), i);
    }
    themeLayout->addRow(tr("Theme:"), themeComboBox);
    appearanceMainLayout->addWidget(themeGroup);

    QGroupBox* uiFontGroup = new QGroupBox(tr("Interface Font"), appearanceContent);
    QFormLayout* uiFontLayout = new QFormLayout(uiFontGroup);
    uiFontFaceComboBox = new QComboBox(uiFontGroup);
    for (const QString& family : QFontDatabase::families())
    {
        uiFontFaceComboBox->addItem(family);
    }
    uiFontLayout->addRow(tr("Font face:"), uiFontFaceComboBox);
    uiFontSizeBox = new QLineEdit(uiFontGroup);
    uiFontSizeBox->setMaximumWidth(50);
    QIntValidator* uiFontSizeValidator = new QIntValidator(6, 72, uiFontSizeBox);
    uiFontSizeBox->setValidator(uiFontSizeValidator);
    uiFontLayout->addRow(tr("Font size:"), uiFontSizeBox);
    appearanceMainLayout->addWidget(uiFontGroup);

    QGroupBox* tabsGroup = new QGroupBox(tr("Tabs"), appearanceContent);
    QFormLayout* tabsLayout = new QFormLayout(tabsGroup);
    tabDisplayComboBox = new QComboBox(tabsGroup);
    tabDisplayComboBox->addItems({tr("Always Show"), tr("Show Two-plus"), tr("Never Show")});
    tabsLayout->addRow(tr("Tab display:"), tabDisplayComboBox);
    closeButtonComboBox = new QComboBox(tabsGroup);
    closeButtonComboBox->addItems({tr("Right"), tr("Left")});
    tabsLayout->addRow(tr("Close button position:"), closeButtonComboBox);
    tabBarPositionComboBox = new QComboBox(tabsGroup);
    tabBarPositionComboBox->addItems({tr("Top"), tr("Bottom")});
    tabsLayout->addRow(tr("Tab bar position:"), tabBarPositionComboBox);
    appearanceMainLayout->addWidget(tabsGroup);

    appearanceMainLayout->addStretch();

    tabWidget->addTab(createScrollContainer(appearanceContent), tr("Appearance"));

    // === Editor Tab ===
    QWidget* editorContent = new QWidget();
    QVBoxLayout* editorMainLayout = new QVBoxLayout(editorContent);

    QGroupBox* fontGroup = new QGroupBox(tr("Font"), editorContent);
    QFormLayout* fontLayout = new QFormLayout(fontGroup);
    fontFaceComboBox = new QComboBox(fontGroup);
    for (const QString& family : QFontDatabase::families())
    {
        if (QFontDatabase::isFixedPitch(family))
        {
            fontFaceComboBox->addItem(family);
        }
    }
    fontLayout->addRow(tr("Font face:"), fontFaceComboBox);
    fontSizeBox = new QLineEdit(fontGroup);
    fontSizeBox->setMaximumWidth(50);
    QIntValidator* fontSizeValidator = new QIntValidator(6, 72, fontSizeBox);
    fontSizeBox->setValidator(fontSizeValidator);
    fontLayout->addRow(tr("Font size:"), fontSizeBox);
    editorMainLayout->addWidget(fontGroup);

    QGroupBox* cursorGroup = new QGroupBox(tr("Cursor"), editorContent);
    QFormLayout* cursorLayout = new QFormLayout(cursorGroup);
    tabWidthSpin = new QSpinBox(cursorGroup);
    tabWidthSpin->setRange(1, 16);
    tabWidthSpin->setValue(4);
    tabWidthSpin->setSuffix(tr(" spaces"));
    cursorLayout->addRow(tr("Tab width:"), tabWidthSpin);
    cursorStyleComboBox = new QComboBox(cursorGroup);
    cursorStyleComboBox->addItems({tr("Line"), tr("Block"), tr("Underline")});
    cursorLayout->addRow(tr("Cursor style:"), cursorStyleComboBox);
    cursorBlinkingCheckBox = new QCheckBox(tr("Cursor blinking"), cursorGroup);
    cursorLayout->addRow(cursorBlinkingCheckBox);
    editorMainLayout->addWidget(cursorGroup);

    QGroupBox* displayGroup = new QGroupBox(tr("Display"), editorContent);
    QVBoxLayout* displayLayout = new QVBoxLayout(displayGroup);
    showLineNumbersCheckBox = new QCheckBox(tr("Show line numbers"), displayGroup);
    scrollPastContentCheckBox = new QCheckBox(tr("Scroll past content"), displayGroup);
    wordWrapCheckBox = new QCheckBox(tr("Word wrap"), displayGroup);
    codeCollapsingCheckBox = new QCheckBox(tr("Code collapsing"), displayGroup);
    showWhitespaceCheckBox = new QCheckBox(tr("Show whitespace"), displayGroup);
    displayLayout->addWidget(showLineNumbersCheckBox);
    displayLayout->addWidget(scrollPastContentCheckBox);
    displayLayout->addWidget(wordWrapCheckBox);
    displayLayout->addWidget(codeCollapsingCheckBox);
    displayLayout->addWidget(showWhitespaceCheckBox);
    displayLayout->addStretch();
    editorMainLayout->addWidget(displayGroup);

    QGroupBox* completionGroup = new QGroupBox(tr("Auto-Completion"), editorContent);
    QFormLayout* completionLayout = new QFormLayout(completionGroup);
    autoCompletionCheckBox = new QCheckBox(tr("Enable auto-completion"), completionGroup);
    completionLayout->addRow(autoCompletionCheckBox);
    autoCompletionThresholdSpin = new QSpinBox(completionGroup);
    autoCompletionThresholdSpin->setRange(1, 10);
    autoCompletionThresholdSpin->setValue(2);
    autoCompletionThresholdSpin->setSuffix(tr(" character(s)"));
    completionLayout->addRow(tr("Trigger after:"), autoCompletionThresholdSpin);
    autoCompletionCaseSensitiveCheckBox = new QCheckBox(tr("Case sensitive"), completionGroup);
    completionLayout->addRow(autoCompletionCaseSensitiveCheckBox);
    editorMainLayout->addWidget(completionGroup);

    QGroupBox* typingGroup = new QGroupBox(tr("Typing"), editorContent);
    QVBoxLayout* typingLayout = new QVBoxLayout(typingGroup);
    autoCloseBracketsCheckBox = new QCheckBox(tr("Auto-close brackets and quotes"), typingGroup);
    typingLayout->addWidget(autoCloseBracketsCheckBox);
    typingLayout->addStretch();
    editorMainLayout->addWidget(typingGroup);

    QGroupBox* editorDisplayGroup = new QGroupBox(tr("Editor Display"), editorContent);
    QVBoxLayout* editorDisplayLayout = new QVBoxLayout(editorDisplayGroup);
    highlightCurrentLineCheckBox = new QCheckBox(tr("Highlight current line"), editorDisplayGroup);
    editorDisplayLayout->addWidget(highlightCurrentLineCheckBox);
    QHBoxLayout* edgeLayout = new QHBoxLayout();
    verticalEdgeCheckBox = new QCheckBox(tr("Show vertical edge at column:"), editorDisplayGroup);
    verticalEdgeColumnSpin = new QSpinBox(editorDisplayGroup);
    verticalEdgeColumnSpin->setRange(1, 300);
    verticalEdgeColumnSpin->setValue(80);
    edgeLayout->addWidget(verticalEdgeCheckBox);
    edgeLayout->addWidget(verticalEdgeColumnSpin);
    edgeLayout->addStretch();
    editorDisplayLayout->addLayout(edgeLayout);
    editorDisplayLayout->addStretch();
    editorMainLayout->addWidget(editorDisplayGroup);

    QGroupBox* autoSaveGroup = new QGroupBox(tr("Auto-Save"), editorContent);
    QFormLayout* autoSaveLayout = new QFormLayout(autoSaveGroup);
    autoSaveCheckBox = new QCheckBox(tr("Enable auto-save"), autoSaveGroup);
    autoSaveLayout->addRow(autoSaveCheckBox);
    autoSaveIntervalSpin = new QSpinBox(autoSaveGroup);
    autoSaveIntervalSpin->setRange(10, 600);
    autoSaveIntervalSpin->setValue(60);
    autoSaveIntervalSpin->setSuffix(tr(" seconds"));
    autoSaveLayout->addRow(tr("Interval:"), autoSaveIntervalSpin);
    connect(autoSaveCheckBox, &QCheckBox::toggled, autoSaveIntervalSpin, &QSpinBox::setEnabled);
    editorMainLayout->addWidget(autoSaveGroup);

    editorMainLayout->addStretch();

    tabWidget->addTab(createScrollContainer(editorContent), tr("Editor"));

    // === Panels Tab ===
    QWidget* panelsContent = new QWidget();
    QVBoxLayout* panelsMainLayout = new QVBoxLayout(panelsContent);

    QGroupBox* terminalGroup = new QGroupBox(tr("Terminal"), panelsContent);
    QFormLayout* terminalLayout = new QFormLayout(terminalGroup);
    terminalPanelPositionComboBox = new QComboBox(terminalGroup);
    terminalPanelPositionComboBox->addItems({tr("Bottom"), tr("Right"), tr("Tab")});
    terminalLayout->addRow(tr("Panel position:"), terminalPanelPositionComboBox);
    terminalPanelMinWidthSpin = new QSpinBox(terminalGroup);
    terminalPanelMinWidthSpin->setRange(100, 2000);
    terminalPanelMinWidthSpin->setValue(350);
    terminalPanelMinWidthSpin->setSuffix(tr(" px"));
    terminalLayout->addRow(tr("Minimum width:"), terminalPanelMinWidthSpin);
    terminalFontFaceComboBox = new QComboBox(terminalGroup);
    for (const QString& family : QFontDatabase::families())
    {
        if (QFontDatabase::isFixedPitch(family))
        {
            terminalFontFaceComboBox->addItem(family);
        }
    }
    terminalLayout->addRow(tr("Font face:"), terminalFontFaceComboBox);
    terminalFontSizeBox = new QLineEdit(terminalGroup);
    terminalFontSizeBox->setMaximumWidth(50);
    QIntValidator* terminalFontSizeValidator = new QIntValidator(6, 72, terminalFontSizeBox);
    terminalFontSizeBox->setValidator(terminalFontSizeValidator);
    terminalLayout->addRow(tr("Font size:"), terminalFontSizeBox);
    panelsMainLayout->addWidget(terminalGroup);

    QGroupBox* projectGroup = new QGroupBox(tr("Project"), panelsContent);
    QFormLayout* projectLayout = new QFormLayout(projectGroup);
    projectPanelPositionComboBox = new QComboBox(projectGroup);
    projectPanelPositionComboBox->addItems({tr("Left"), tr("Right")});
    projectLayout->addRow(tr("Panel position:"), projectPanelPositionComboBox);
    showHiddenFilesCheckBox = new QCheckBox(tr("Show hidden files"), projectGroup);
    projectLayout->addRow(showHiddenFilesCheckBox);
    panelsMainLayout->addWidget(projectGroup);
    panelsMainLayout->addStretch();

    tabWidget->addTab(createScrollContainer(panelsContent), tr("Panels"));

    mainLayout->addWidget(tabWidget);

    QHBoxLayout* buttonLayout = new QHBoxLayout();
    QPushButton* okButton = new QPushButton(tr("OK"), this);
    QPushButton* cancelButton = new QPushButton(tr("Cancel"), this);
    buttonLayout->addStretch();
    buttonLayout->addWidget(okButton);
    buttonLayout->addWidget(cancelButton);
    mainLayout->addLayout(buttonLayout);

    connect(okButton, &QPushButton::clicked, this, &OptionsDialog::accept);
    connect(cancelButton, &QPushButton::clicked, this, &QDialog::reject);
}

void OptionsDialog::loadSettings()
{
    const auto& s = SettingsManager::instance();
    startupComboBox->setCurrentIndex(static_cast<int>(s.startupMode()));
    int savedTheme = static_cast<int>(s.theme());
    for (int i = 0; i < themeComboBox->count(); ++i)
    {
        if (themeComboBox->itemData(i).toInt() == savedTheme)
        {
            themeComboBox->setCurrentIndex(i);
            break;
        }
    }
    defaultFormatComboBox->setCurrentIndex(s.defaultFormat());
    closeButtonComboBox->setCurrentIndex(static_cast<int>(s.closeButtonMode()));
    tabDisplayComboBox->setCurrentIndex(static_cast<int>(s.tabDisplayMode()));
    tabWidthSpin->setValue(s.tabWidth());
    cursorStyleComboBox->setCurrentIndex(static_cast<int>(s.cursorStyle()));
    cursorBlinkingCheckBox->setChecked(s.cursorBlinking());
    highlightCurrentLineCheckBox->setChecked(s.highlightCurrentLine());
    tabBarPositionComboBox->setCurrentIndex(static_cast<int>(s.tabBarPosition()));
    terminalPanelPositionComboBox->setCurrentIndex(static_cast<int>(s.terminalPanelPosition()));
    terminalPanelMinWidthSpin->setValue(s.terminalPanelMinWidth());
    QString termFont = s.terminalFontFamily();
    int termFontIndex = terminalFontFaceComboBox->findText(termFont);
    if (termFontIndex >= 0)
        terminalFontFaceComboBox->setCurrentIndex(termFontIndex);
    terminalFontSizeBox->setText(QString::number(s.terminalFontSize()));
    projectPanelPositionComboBox->setCurrentIndex(static_cast<int>(s.projectPanelPosition()));
    showHiddenFilesCheckBox->setChecked(s.showHiddenFiles());
    verticalEdgeCheckBox->setChecked(s.verticalEdgeEnabled());
    verticalEdgeColumnSpin->setValue(s.verticalEdgeColumn());
    defaultEncodingComboBox->setCurrentIndex(s.defaultEncoding());
    QString font = s.defaultFontFamily();
    int index = fontFaceComboBox->findText(font);
    if (index >= 0)
        fontFaceComboBox->setCurrentIndex(index);
    fontSizeBox->setText(QString::number(s.defaultFontSize()));
    QString uiFont = s.uiFontFamily();
    int uiIndex = uiFontFaceComboBox->findText(uiFont);
    if (uiIndex >= 0)
        uiFontFaceComboBox->setCurrentIndex(uiIndex);
    uiFontSizeBox->setText(QString::number(s.uiFontSize()));
    showLineNumbersCheckBox->setChecked(s.showLineNumbers());
    scrollPastContentCheckBox->setChecked(s.scrollPastContent());
    wordWrapCheckBox->setChecked(s.wordWrap());
    codeCollapsingCheckBox->setChecked(s.codeCollapsing());
    showWhitespaceCheckBox->setChecked(s.showWhitespace());
    autoCompletionCheckBox->setChecked(s.autoCompletionEnabled());
    autoCompletionThresholdSpin->setValue(s.autoCompletionThreshold());
    autoCompletionCaseSensitiveCheckBox->setChecked(s.autoCompletionCaseSensitive());
    autoCloseBracketsCheckBox->setChecked(s.autoCloseBrackets());
    autoSaveCheckBox->setChecked(s.autoSaveEnabled());
    autoSaveIntervalSpin->setValue(s.autoSaveInterval());
    autoSaveIntervalSpin->setEnabled(autoSaveCheckBox->isChecked());
}

void OptionsDialog::saveSettings()
{
    SettingsManager::instance().setStartupMode(static_cast<StartupMode>(startupComboBox->currentIndex()));
    SettingsManager::instance().setTheme(static_cast<ThemeId>(themeComboBox->currentData().toInt()));
    SettingsManager::instance().setDefaultFormat(defaultFormatComboBox->currentIndex());
    SettingsManager::instance().setCloseButtonMode(static_cast<CloseButtonMode>(closeButtonComboBox->currentIndex()));
    SettingsManager::instance().setTabDisplayMode(static_cast<TabDisplayMode>(tabDisplayComboBox->currentIndex()));
    SettingsManager::instance().setTabWidth(tabWidthSpin->value());
    SettingsManager::instance().setCursorStyle(static_cast<CursorStyle>(cursorStyleComboBox->currentIndex()));
    SettingsManager::instance().setCursorBlinking(cursorBlinkingCheckBox->isChecked());
    SettingsManager::instance().setHighlightCurrentLine(highlightCurrentLineCheckBox->isChecked());
    SettingsManager::instance().setTabBarPosition(static_cast<TabBarPosition>(tabBarPositionComboBox->currentIndex()));
    SettingsManager::instance().setTerminalPanelPosition(static_cast<TerminalPanelPosition>(terminalPanelPositionComboBox->currentIndex()));
    SettingsManager::instance().setTerminalPanelMinWidth(terminalPanelMinWidthSpin->value());
    SettingsManager::instance().setTerminalFontFamily(terminalFontFaceComboBox->currentText());
    SettingsManager::instance().setTerminalFontSize(qMax(1, terminalFontSizeBox->text().toInt()));
    SettingsManager::instance().setProjectPanelPosition(static_cast<ProjectPanelPosition>(projectPanelPositionComboBox->currentIndex()));
    SettingsManager::instance().setShowHiddenFiles(showHiddenFilesCheckBox->isChecked());
    SettingsManager::instance().setVerticalEdgeEnabled(verticalEdgeCheckBox->isChecked());
    SettingsManager::instance().setVerticalEdgeColumn(verticalEdgeColumnSpin->value());
    SettingsManager::instance().setDefaultFontFamily(fontFaceComboBox->currentText());
    SettingsManager::instance().setDefaultFontSize(qMax(1, fontSizeBox->text().toInt()));
    SettingsManager::instance().setUiFontFamily(uiFontFaceComboBox->currentText());
    SettingsManager::instance().setUiFontSize(qMax(1, uiFontSizeBox->text().toInt()));
    SettingsManager::instance().setShowLineNumbers(showLineNumbersCheckBox->isChecked());
    SettingsManager::instance().setScrollPastContent(scrollPastContentCheckBox->isChecked());
    SettingsManager::instance().setWordWrap(wordWrapCheckBox->isChecked());
    SettingsManager::instance().setCodeCollapsing(codeCollapsingCheckBox->isChecked());
    SettingsManager::instance().setShowWhitespace(showWhitespaceCheckBox->isChecked());
    SettingsManager::instance().setAutoCompletionEnabled(autoCompletionCheckBox->isChecked());
    SettingsManager::instance().setAutoCompletionThreshold(autoCompletionThresholdSpin->value());
    SettingsManager::instance().setAutoCompletionCaseSensitive(autoCompletionCaseSensitiveCheckBox->isChecked());
    SettingsManager::instance().setAutoCloseBrackets(autoCloseBracketsCheckBox->isChecked());
    SettingsManager::instance().setAutoSaveEnabled(autoSaveCheckBox->isChecked());
    SettingsManager::instance().setAutoSaveInterval(autoSaveIntervalSpin->value());
    SettingsManager::instance().setDefaultEncoding(defaultEncodingComboBox->currentIndex());
}

void OptionsDialog::accept()
{
    saveSettings();
    QDialog::accept();
}
