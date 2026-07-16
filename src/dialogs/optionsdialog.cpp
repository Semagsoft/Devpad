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
#include "widgets/themepreviewwidget.h"

#include "defaultsyntax.h"
#include "encodingutils.h"
#include "lsp/lspservermanager.h"
#include "settingsmanager.h"
#include "theme.h"

#include <QComboBox>
#include <QCheckBox>
#include <QGroupBox>
#include <QLineEdit>
#include <QPushButton>
#include <QSpinBox>
#include <QTabWidget>
#include <QTableWidget>
#include <QPushButton>
#include <QSpinBox>
#include <QTabWidget>
#include <QTableWidget>
#include <QColorDialog>
#include <QFontDatabase>
#include <QHeaderView>
#include <QLabel>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QIntValidator>
#include <QMessageBox>
#include <QScrollArea>
#include <QVBoxLayout>

OptionsDialog::OptionsDialog(QWidget* parent)
    : QDialog(parent), m_geometrySettings("Options")
{
    setWindowTitle(tr("Options"));
    setModal(true);
    setMinimumSize(440, 400);
    setupUI();
    loadSettings();
    m_geometrySettings.restoreGeometry(this);
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
    setupGeneralTab();
    setupAppearanceTab();
    setupEditorTab();
    setupPanelsTab();
    setupLspTab();

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

void OptionsDialog::setupGeneralTab()
{
    QWidget* content = new QWidget();
    QVBoxLayout* mainLayout = new QVBoxLayout(content);

    QGroupBox* startupGroup = new QGroupBox(tr("Startup"), content);
    QFormLayout* startupLayout = new QFormLayout(startupGroup);
    startupComboBox = new QComboBox(startupGroup);
    startupComboBox->addItems({tr("New File"), tr("Restore Session"), tr("Open Last File"), tr("Empty")});
    startupLayout->addRow(tr("Startup mode:"), startupComboBox);
    mainLayout->addWidget(startupGroup);

    QGroupBox* defaultsGroup = new QGroupBox(tr("Defaults"), content);
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
    mainLayout->addWidget(defaultsGroup);
    mainLayout->addStretch();

    tabWidget->addTab(createScrollContainer(content), tr("General"));
}

void OptionsDialog::setupAppearanceTab()
{
    QWidget* content = new QWidget();
    QVBoxLayout* mainLayout = new QVBoxLayout(content);

    QGroupBox* themeGroup = new QGroupBox(tr("Theme"), content);
    QFormLayout* themeLayout = new QFormLayout(themeGroup);
    themeComboBox = new QComboBox(themeGroup);
    for (auto id : allBuiltInThemes())
    {
        themeComboBox->addItem(themeDisplayName(id), static_cast<int>(id));
    }
    // Add custom user themes
    QStringList customThemes = customThemeNames();
    if (!customThemes.isEmpty()) {
        themeComboBox->insertSeparator(themeComboBox->count());
        for (const QString &name : customThemes) {
            themeComboBox->addItem(name, -1);
        }
    }
    themeLayout->addRow(tr("Theme:"), themeComboBox);

    themePreview = new ThemePreviewWidget(themeGroup);
    themePreview->setFixedHeight(110);
    themeLayout->addRow(themePreview);

    QHBoxLayout *accentLayout = new QHBoxLayout();
    accentColorButton = new QPushButton(themeGroup);
    accentColorButton->setFixedSize(24, 24);
    accentColorButton->setCursor(Qt::PointingHandCursor);
    QLabel *accentLabel = new QLabel(tr("Accent color:"), themeGroup);
    QPushButton *clearAccentBtn = new QPushButton(tr("Clear"), themeGroup);
    accentLayout->addWidget(accentColorButton);
    accentLayout->addWidget(clearAccentBtn);
    accentLayout->addStretch();
    themeLayout->addRow(accentLabel, accentLayout);

    connect(themeComboBox, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [this]() {
        int data = themeComboBox->currentData().toInt();
        if (data >= 0) {
            SettingsManager::instance().setTheme(static_cast<ThemeId>(data));
        }
        updateThemePreview();
        emit themeChanged();
    });

    connect(accentColorButton, &QPushButton::clicked, this, [this]() {
        QColor chosen = QColorDialog::getColor(m_accentColor, this, tr("Choose Accent Color"));
        if (chosen.isValid()) {
            m_accentColor = chosen;
            SettingsManager::instance().setAccentColor(m_accentColor);
            QString bg = chosen.name();
            accentColorButton->setStyleSheet(QString("background-color: %1; border: 1px solid gray; border-radius: 3px;").arg(bg));
            updateThemePreview();
            emit themeChanged();
        }
    });

    connect(clearAccentBtn, &QPushButton::clicked, this, [this]() {
        m_accentColor = QColor();
        SettingsManager::instance().clearAccentColor();
        accentColorButton->setStyleSheet(QString());
        updateThemePreview();
        emit themeChanged();
    });

    mainLayout->addWidget(themeGroup);

    QGroupBox* uiFontGroup = new QGroupBox(tr("Interface Font"), content);
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
    mainLayout->addWidget(uiFontGroup);

    QGroupBox* tabsGroup = new QGroupBox(tr("Tabs"), content);
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
    mainLayout->addWidget(tabsGroup);

    mainLayout->addStretch();

    tabWidget->addTab(createScrollContainer(content), tr("Appearance"));
}

void OptionsDialog::setupEditorTab()
{
    QWidget* content = new QWidget();
    QVBoxLayout* mainLayout = new QVBoxLayout(content);

    QGroupBox* fontGroup = new QGroupBox(tr("Font"), content);
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
    mainLayout->addWidget(fontGroup);

    QGroupBox* cursorGroup = new QGroupBox(tr("Cursor"), content);
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
    mainLayout->addWidget(cursorGroup);

    QGroupBox* displayGroup = new QGroupBox(tr("Display"), content);
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
    mainLayout->addWidget(displayGroup);

    QGroupBox* completionGroup = new QGroupBox(tr("Auto-Completion"), content);
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

    QGroupBox* snippetGroup = new QGroupBox(tr("Snippets"), content);
    QVBoxLayout* snippetLayout = new QVBoxLayout(snippetGroup);
    snippetsCheckBox = new QCheckBox(tr("Enable snippets"), snippetGroup);
    snippetLayout->addWidget(snippetsCheckBox);
    predictiveSnippetsCheckBox = new QCheckBox(tr("Show snippets in auto-completion (predictive)"), snippetGroup);
    snippetLayout->addWidget(predictiveSnippetsCheckBox);
    snippetLayout->addStretch();
    mainLayout->addWidget(snippetGroup);

    mainLayout->addWidget(completionGroup);

    QGroupBox* typingGroup = new QGroupBox(tr("Typing"), content);
    QVBoxLayout* typingLayout = new QVBoxLayout(typingGroup);
    autoCloseBracketsCheckBox = new QCheckBox(tr("Auto-close brackets and quotes"), typingGroup);
    typingLayout->addWidget(autoCloseBracketsCheckBox);
    typingLayout->addStretch();
    mainLayout->addWidget(typingGroup);

    QGroupBox* editorDisplayGroup = new QGroupBox(tr("Editor Display"), content);
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
    mainLayout->addWidget(editorDisplayGroup);

    QGroupBox* autoSaveGroup = new QGroupBox(tr("Auto-Save"), content);
    QFormLayout* autoSaveLayout = new QFormLayout(autoSaveGroup);
    autoSaveCheckBox = new QCheckBox(tr("Enable auto-save"), autoSaveGroup);
    autoSaveLayout->addRow(autoSaveCheckBox);
    autoSaveIntervalSpin = new QSpinBox(autoSaveGroup);
    autoSaveIntervalSpin->setRange(10, 600);
    autoSaveIntervalSpin->setValue(60);
    autoSaveIntervalSpin->setSuffix(tr(" seconds"));
    autoSaveLayout->addRow(tr("Interval:"), autoSaveIntervalSpin);
    connect(autoSaveCheckBox, &QCheckBox::toggled, autoSaveIntervalSpin, &QSpinBox::setEnabled);
    mainLayout->addWidget(autoSaveGroup);

    mainLayout->addStretch();

    tabWidget->addTab(createScrollContainer(content), tr("Editor"));
}

void OptionsDialog::setupPanelsTab()
{
    QWidget* content = new QWidget();
    QVBoxLayout* mainLayout = new QVBoxLayout(content);

    QGroupBox* terminalGroup = new QGroupBox(tr("Terminal"), content);
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
    mainLayout->addWidget(terminalGroup);

    QGroupBox* projectGroup = new QGroupBox(tr("Project"), content);
    QFormLayout* projectLayout = new QFormLayout(projectGroup);
    projectPanelPositionComboBox = new QComboBox(projectGroup);
    projectPanelPositionComboBox->addItems({tr("Left"), tr("Right")});
    projectLayout->addRow(tr("Panel position:"), projectPanelPositionComboBox);
    showHiddenFilesCheckBox = new QCheckBox(tr("Show hidden files"), projectGroup);
    projectLayout->addRow(showHiddenFilesCheckBox);
    useGitIgnoreCheckBox = new QCheckBox(tr("Use .gitignore filtering"), projectGroup);
    projectLayout->addRow(useGitIgnoreCheckBox);
    mainLayout->addWidget(projectGroup);
    mainLayout->addStretch();

    tabWidget->addTab(createScrollContainer(content), tr("Panels"));
}

void OptionsDialog::setupLspTab()
{
    QWidget* content = new QWidget();
    QVBoxLayout* mainLayout = new QVBoxLayout(content);

    QGroupBox* generalGroup = new QGroupBox(tr("Language Server Protocol"), content);
    QVBoxLayout* generalLayout = new QVBoxLayout(generalGroup);
    lspEnabledCheckBox = new QCheckBox(tr("Enable LSP (Language Server Protocol)"), generalGroup);
    generalLayout->addWidget(lspEnabledCheckBox);
    lspShowErrorListCheckBox = new QCheckBox(tr("Show error list"), generalGroup);
    generalLayout->addWidget(lspShowErrorListCheckBox);
    QFormLayout* thresholdLayout = new QFormLayout();
    lspCompletionTriggerSpin = new QSpinBox(generalGroup);
    lspCompletionTriggerSpin->setRange(1, 10);
    lspCompletionTriggerSpin->setValue(2);
    lspCompletionTriggerSpin->setSuffix(tr(" character(s)"));
    thresholdLayout->addRow(tr("Completion trigger after:"), lspCompletionTriggerSpin);
    generalLayout->addLayout(thresholdLayout);
    mainLayout->addWidget(generalGroup);

    QGroupBox* serverGroup = new QGroupBox(tr("Server Commands"), content);
    QVBoxLayout* serverLayout = new QVBoxLayout(serverGroup);
    lspServerTable = new QTableWidget(serverGroup);
    lspServerTable->setColumnCount(3);
    lspServerTable->setHorizontalHeaderLabels({tr("Language"), tr("Command"), tr("Arguments")});
    lspServerTable->horizontalHeader()->setStretchLastSection(true);
    lspServerTable->horizontalHeader()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
    lspServerTable->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Stretch);
    lspServerTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    lspServerTable->setSelectionMode(QAbstractItemView::SingleSelection);
    serverLayout->addWidget(lspServerTable);
    mainLayout->addWidget(serverGroup);

    mainLayout->addStretch();
    tabWidget->addTab(createScrollContainer(content), tr("LSP"));
}

void OptionsDialog::updateThemePreview()
{
    ThemeColors colors;
    int data = themeComboBox->currentData().toInt();
    if (data < 0) {
        // Custom theme
        QString name = themeComboBox->currentText();
        colors = getCustomThemeColors(name);
    } else {
        colors = getThemeColors(static_cast<ThemeId>(data));
    }
    if (m_accentColor.isValid()) {
        colors.accent = m_accentColor;
        colors.resolve();
    }
    themePreview->setThemeColors(colors);
}

void OptionsDialog::loadSettings()
{
    const auto& s = SettingsManager::instance();
    startupComboBox->setCurrentIndex(static_cast<int>(s.startupMode()));
    m_accentColor = s.accentColor();
    if (m_accentColor.isValid()) {
        accentColorButton->setStyleSheet(QString("background-color: %1; border: 1px solid gray; border-radius: 3px;").arg(m_accentColor.name()));
    }
    ThemeId savedThemeId = s.theme();
    bool found = false;
    for (int i = 0; i < themeComboBox->count(); ++i)
    {
        if (themeComboBox->itemData(i).toInt() == static_cast<int>(savedThemeId))
        {
            themeComboBox->setCurrentIndex(i);
            found = true;
            break;
        }
    }
    if (!found && themeComboBox->count() > 0) {
        themeComboBox->setCurrentIndex(0);
    }
    updateThemePreview();
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
    useGitIgnoreCheckBox->setChecked(s.useGitIgnore());
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
    snippetsCheckBox->setChecked(s.snippetsEnabled());
    predictiveSnippetsCheckBox->setChecked(s.predictiveSnippets());
    autoSaveCheckBox->setChecked(s.autoSaveEnabled());
    autoSaveIntervalSpin->setValue(s.autoSaveInterval());
    autoSaveIntervalSpin->setEnabled(autoSaveCheckBox->isChecked());

    lspEnabledCheckBox->setChecked(s.lspEnabled());
    lspShowErrorListCheckBox->setChecked(s.lspShowErrorList());
    lspCompletionTriggerSpin->setValue(s.lspCompletionTriggerChars());

    // Populate LSP server table with default + custom entries
    auto defaultCmds = lsp::LspServerManager::defaultServerCommands();
    QSet<QString> seen;
    lspServerTable->setRowCount(0);
    for (auto it = defaultCmds.constBegin(); it != defaultCmds.constEnd(); ++it) {
        int row = lspServerTable->rowCount();
        lspServerTable->insertRow(row);
        QString lang = it.key();
        QString cmd = s.lspServerCommand(lang);
        QStringList args = s.lspServerArgs(lang);
        if (cmd.isEmpty()) cmd = it.value();
        if (args.isEmpty()) args = lsp::LspServerManager::defaultServerArgs(lang);
        lspServerTable->setItem(row, 0, new QTableWidgetItem(lang));
        lspServerTable->setItem(row, 1, new QTableWidgetItem(cmd));
        lspServerTable->setItem(row, 2, new QTableWidgetItem(args.join(' ')));
        seen.insert(lang);
    }
}

void OptionsDialog::saveSettings()
{
    int themeData = themeComboBox->currentData().toInt();
    if (themeData >= 0) {
        SettingsManager::instance().setTheme(static_cast<ThemeId>(themeData));
    }
    SettingsManager::instance().setStartupMode(static_cast<StartupMode>(startupComboBox->currentIndex()));
    if (m_accentColor.isValid())
        SettingsManager::instance().setAccentColor(m_accentColor);
    else
        SettingsManager::instance().clearAccentColor();
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
    SettingsManager::instance().setUseGitIgnore(useGitIgnoreCheckBox->isChecked());
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
    SettingsManager::instance().setSnippetsEnabled(snippetsCheckBox->isChecked());
    SettingsManager::instance().setPredictiveSnippets(predictiveSnippetsCheckBox->isChecked());
    SettingsManager::instance().setAutoSaveEnabled(autoSaveCheckBox->isChecked());
    SettingsManager::instance().setAutoSaveInterval(autoSaveIntervalSpin->value());
    SettingsManager::instance().setDefaultEncoding(defaultEncodingComboBox->currentIndex());

    SettingsManager::instance().setLspEnabled(lspEnabledCheckBox->isChecked());
    SettingsManager::instance().setLspShowErrorList(lspShowErrorListCheckBox->isChecked());
    SettingsManager::instance().setLspCompletionTriggerChars(lspCompletionTriggerSpin->value());

    // Save LSP server table
    for (int i = 0; i < lspServerTable->rowCount(); ++i) {
        QTableWidgetItem* langItem = lspServerTable->item(i, 0);
        QTableWidgetItem* cmdItem = lspServerTable->item(i, 1);
        QTableWidgetItem* argsItem = lspServerTable->item(i, 2);
        if (!langItem || !cmdItem) continue;
        QString lang = langItem->text().trimmed();
        QString cmd = cmdItem->text().trimmed();
        QStringList args;
        if (argsItem && !argsItem->text().trimmed().isEmpty())
            args = argsItem->text().trimmed().split(' ', Qt::SkipEmptyParts);
        auto defaultCmds = lsp::LspServerManager::defaultServerCommands();
        if (cmd != defaultCmds.value(lang)) {
            SettingsManager::instance().setLspServerCommand(lang, cmd);
        }
        auto defaultArgs = lsp::LspServerManager::defaultServerArgs(lang);
        if (args != defaultArgs) {
            SettingsManager::instance().setLspServerArgs(lang, args);
        }
    }
}

void OptionsDialog::accept()
{
    m_geometrySettings.saveGeometry(this);
    saveSettings();
    QDialog::accept();
}

void OptionsDialog::reject()
{
    m_geometrySettings.saveGeometry(this);
    QDialog::reject();
}
