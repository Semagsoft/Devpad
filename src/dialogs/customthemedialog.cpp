#include "customthemedialog.h"

#include "settingsmanager.h"
#include "theme.h"
#include "widgets/themepreviewwidget.h"

#include <QCheckBox>
#include <QColorDialog>
#include <QDialogButtonBox>
#include <QDir>
#include <QFile>
#include <QFormLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QInputDialog>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLabel>
#include <QListWidget>
#include <QMessageBox>
#include <QPushButton>
#include <QScrollArea>
#include <QStandardPaths>
#include <QVBoxLayout>

namespace
{
// Core (editable) color keys, matching parseThemeJson() in theme.cpp.
constexpr int NumCoreColors = 19;
const char* CoreColorKeys[NumCoreColors] = {
    "background",   "foreground",      "surfaceBg", "surfaceFg", "accent", "border", "caret",    "lineHighlight", "marginBg",     "marginFg",
    "foldMarginBg", "foldMarginBgAlt", "comment",   "keyword",   "string", "number", "operator", "function",      "preprocessor",
};

QString colorToJson(const QColor& c)
{
    return c.isValid() ? c.name() : QString();
}

// jsonColorOr is unused: colors are read directly from the form swatches.
} // namespace

CustomThemeDialog::CustomThemeDialog(QWidget* parent) : QDialog(parent)
{
    setWindowTitle(tr("Custom Theme Editor"));
    setMinimumSize(760, 560);

    QString configDir = QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation);
    m_themesDir = configDir + QStringLiteral("/themes");

    setupUi();
    refreshThemeList();
}

CustomThemeDialog::~CustomThemeDialog() = default;

void CustomThemeDialog::setupUi()
{
    auto* mainLayout = new QHBoxLayout(this);

    // ── Left: theme list ─────────────────────────────────────────
    QWidget* leftPane = new QWidget(this);
    auto* leftLayout = new QVBoxLayout(leftPane);
    leftLayout->setContentsMargins(0, 0, 0, 0);

    m_themeList = new QListWidget(leftPane);
    m_themeList->setSelectionMode(QAbstractItemView::SingleSelection);
    leftLayout->addWidget(new QLabel(tr("Custom themes"), leftPane));
    leftLayout->addWidget(m_themeList);

    auto* listBtns = new QHBoxLayout();
    m_newBtn = new QPushButton(tr("New"), leftPane);
    m_deleteBtn = new QPushButton(tr("Delete"), leftPane);
    m_deleteBtn->setEnabled(false);
    listBtns->addWidget(m_newBtn);
    listBtns->addWidget(m_deleteBtn);
    listBtns->addStretch();
    leftLayout->addLayout(listBtns);

    mainLayout->addWidget(leftPane, 0);

    // ── Right: theme editor ──────────────────────────────────────
    QWidget* rightPane = new QWidget(this);
    auto* rightLayout = new QVBoxLayout(rightPane);

    auto* formGroup = new QGroupBox(tr("Theme properties"), rightPane);
    auto* formLayout = new QFormLayout(formGroup);

    m_nameEdit = new QLineEdit(rightPane);
    m_nameEdit->setPlaceholderText(tr("Theme name"));
    formLayout->addRow(tr("Name:"), m_nameEdit);

    m_isDarkCheck = new QCheckBox(tr("Dark theme"), rightPane);
    formLayout->addRow(tr("Kind:"), m_isDarkCheck);

    rightLayout->addWidget(formGroup);

    // Color pickers (scrollable, 19 core colors)
    auto* scroll = new QScrollArea(rightPane);
    scroll->setWidgetResizable(true);
    scroll->setFrameShape(QFrame::NoFrame);

    QWidget* colorsContainer = new QWidget(scroll);
    auto* colorsLayout = new QFormLayout(colorsContainer);

    for (int i = 0; i < NumCoreColors; ++i)
    {
        QString key = QString::fromLatin1(CoreColorKeys[i]);
        QPushButton* swatch = new QPushButton(colorsContainer);
        swatch->setFixedSize(28, 20);
        swatch->setCursor(Qt::PointingHandCursor);
        // Human-friendly label: capitalize the first letter of the key.
        QString label = key;
        label[0] = label[0].toUpper();
        colorsLayout->addRow(tr(label.toUtf8().constData()), swatch);
        m_colorSwatches.insert(key, swatch);

        connect(swatch, &QPushButton::clicked, this, [this, swatch]() { pickColor(swatch); });
    }

    scroll->setWidget(colorsContainer);
    rightLayout->addWidget(scroll, 1);

    m_preview = new ThemePreviewWidget(rightPane);
    m_preview->setFixedHeight(140);
    rightLayout->addWidget(m_preview);

    mainLayout->addWidget(rightPane, 1);

    // ── Bottom buttons ───────────────────────────────────────────
    m_buttonBox = new QDialogButtonBox(QDialogButtonBox::Close, this);
    m_saveBtn = m_buttonBox->addButton(tr("Save"), QDialogButtonBox::AcceptRole);
    m_saveApplyBtn = m_buttonBox->addButton(tr("Save & Apply"), QDialogButtonBox::ApplyRole);
    m_saveApplyBtn->setEnabled(false);

    auto* bottomLayout = new QHBoxLayout();
    bottomLayout->addStretch();
    bottomLayout->addWidget(m_saveBtn);
    bottomLayout->addWidget(m_saveApplyBtn);
    bottomLayout->addWidget(m_buttonBox);
    rightLayout->addLayout(bottomLayout);

    // ── Connections ──────────────────────────────────────────────
    connect(m_themeList, &QListWidget::currentItemChanged, this,
            [this](QListWidgetItem* current, QListWidgetItem*)
            {
                if (current)
                    loadTheme(current->text());
                else
                {
                    m_editingName.clear();
                    m_saveApplyBtn->setEnabled(false);
                }
                m_deleteBtn->setEnabled(current != nullptr);
            });
    connect(m_nameEdit, &QLineEdit::textEdited, this, [this]() { m_saveApplyBtn->setEnabled(!m_nameEdit->text().trimmed().isEmpty()); });
    connect(m_isDarkCheck, &QCheckBox::toggled, this, &CustomThemeDialog::updatePreview);

    connect(m_newBtn, &QPushButton::clicked, this, &CustomThemeDialog::newTheme);
    connect(m_deleteBtn, &QPushButton::clicked, this, &CustomThemeDialog::deleteTheme);
    connect(m_saveBtn, &QPushButton::clicked, this, [this]() { (void)saveTheme(false); });
    connect(m_saveApplyBtn, &QPushButton::clicked, this, [this]() { (void)saveTheme(true); });
    connect(m_buttonBox, &QDialogButtonBox::rejected, this, &CustomThemeDialog::reject);
    connect(m_buttonBox, &QDialogButtonBox::accepted, this,
            [this]()
            {
                (void)saveTheme(false);
                accept();
            });
}

void CustomThemeDialog::refreshThemeList()
{
    m_themeList->blockSignals(true);
    m_themeList->clear();
    for (const QString& name : customThemeNames())
        m_themeList->addItem(name);
    m_themeList->blockSignals(false);

    if (m_themeList->count() > 0 && !m_themeList->currentItem())
        m_themeList->setCurrentRow(0);
}

void CustomThemeDialog::loadTheme(const QString& name)
{
    m_editingName = name;
    ThemeColors colors = getCustomThemeColors(name);
    applyColorsToForm(colors);
    m_saveApplyBtn->setEnabled(true);
}

void CustomThemeDialog::applyColorsToForm(const ThemeColors& colors)
{
    QMap<QString, QColor> values = {
        {QStringLiteral("background"), colors.background},
        {QStringLiteral("foreground"), colors.foreground},
        {QStringLiteral("surfaceBg"), colors.surfaceBg},
        {QStringLiteral("surfaceFg"), colors.surfaceFg},
        {QStringLiteral("accent"), colors.accent},
        {QStringLiteral("border"), colors.border},
        {QStringLiteral("caret"), colors.caret},
        {QStringLiteral("lineHighlight"), colors.lineHighlight},
        {QStringLiteral("marginBg"), colors.marginBg},
        {QStringLiteral("marginFg"), colors.marginFg},
        {QStringLiteral("foldMarginBg"), colors.foldMarginBg},
        {QStringLiteral("foldMarginBgAlt"), colors.foldMarginBgAlt},
        {QStringLiteral("comment"), colors.comment},
        {QStringLiteral("keyword"), colors.keyword},
        {QStringLiteral("string"), colors.string},
        {QStringLiteral("number"), colors.number},
        {QStringLiteral("operator"), colors.operator_},
        {QStringLiteral("function"), colors.function},
        {QStringLiteral("preprocessor"), colors.preprocessor},
    };

    m_nameEdit->blockSignals(true);
    m_nameEdit->setText(colors.name);
    m_nameEdit->blockSignals(false);

    m_isDarkCheck->blockSignals(true);
    m_isDarkCheck->setChecked(colors.isDark);
    m_isDarkCheck->blockSignals(false);

    for (auto it = values.begin(); it != values.end(); ++it)
    {
        if (QPushButton* swatch = m_colorSwatches.value(it.key()))
        {
            QString bg = it.value().name();
            swatch->setStyleSheet(QString("background-color: %1; border: 1px solid %2; border-radius: 3px;")
                                      .arg(bg)
                                      .arg(it.value().lightness() < 128 ? QStringLiteral("gray") : QStringLiteral("darkgray")));
        }
    }

    updatePreview();
}

ThemeColors CustomThemeDialog::currentColors() const
{
    ThemeColors c;
    c.name = m_nameEdit->text().trimmed();
    if (c.name.isEmpty())
        c.name = QStringLiteral("Custom");
    c.isDark = m_isDarkCheck->isChecked();

    for (int i = 0; i < NumCoreColors; ++i)
    {
        QString key = QString::fromLatin1(CoreColorKeys[i]);
        QPushButton* swatch = m_colorSwatches.value(key);
        QColor col;
        if (swatch)
        {
            QString sheet = swatch->styleSheet();
            // Extract the background-color value from the style sheet.
            const QString prefix = QStringLiteral("background-color: ");
            int idx = sheet.indexOf(prefix);
            if (idx >= 0)
            {
                int end = sheet.indexOf(QLatin1Char(';'), idx);
                if (end < 0)
                    end = sheet.size();
                QString colorStr = sheet.mid(idx + prefix.size(), end - (idx + prefix.size())).trimmed();
                col = QColor(colorStr);
            }
        }
        if (!col.isValid())
            col = QColor(Qt::black); // should not happen

        if (key == QStringLiteral("background"))
            c.background = col;
        else if (key == QStringLiteral("foreground"))
            c.foreground = col;
        else if (key == QStringLiteral("surfaceBg"))
            c.surfaceBg = col;
        else if (key == QStringLiteral("surfaceFg"))
            c.surfaceFg = col;
        else if (key == QStringLiteral("accent"))
            c.accent = col;
        else if (key == QStringLiteral("border"))
            c.border = col;
        else if (key == QStringLiteral("caret"))
            c.caret = col;
        else if (key == QStringLiteral("lineHighlight"))
            c.lineHighlight = col;
        else if (key == QStringLiteral("marginBg"))
            c.marginBg = col;
        else if (key == QStringLiteral("marginFg"))
            c.marginFg = col;
        else if (key == QStringLiteral("foldMarginBg"))
            c.foldMarginBg = col;
        else if (key == QStringLiteral("foldMarginBgAlt"))
            c.foldMarginBgAlt = col;
        else if (key == QStringLiteral("comment"))
            c.comment = col;
        else if (key == QStringLiteral("keyword"))
            c.keyword = col;
        else if (key == QStringLiteral("string"))
            c.string = col;
        else if (key == QStringLiteral("number"))
            c.number = col;
        else if (key == QStringLiteral("operator"))
            c.operator_ = col;
        else if (key == QStringLiteral("function"))
            c.function = col;
        else if (key == QStringLiteral("preprocessor"))
            c.preprocessor = col;
    }
    return c;
}

void CustomThemeDialog::pickColor(QPushButton* swatch)
{
    QColor current;
    QString sheet = swatch->styleSheet();
    const QString prefix = QStringLiteral("background-color: ");
    int idx = sheet.indexOf(prefix);
    if (idx >= 0)
    {
        int end = sheet.indexOf(QLatin1Char(';'), idx);
        if (end < 0)
            end = sheet.size();
        current = QColor(sheet.mid(idx + prefix.size(), end - (idx + prefix.size())).trimmed());
    }
    if (!current.isValid())
        current = QColor(Qt::white);

    QColor chosen = QColorDialog::getColor(current, this, tr("Choose Color"));
    if (!chosen.isValid())
        return;

    QString bg = chosen.name();
    swatch->setStyleSheet(QString("background-color: %1; border: 1px solid %2; border-radius: 3px;")
                              .arg(bg)
                              .arg(chosen.lightness() < 128 ? QStringLiteral("gray") : QStringLiteral("darkgray")));
    updatePreview();
}

void CustomThemeDialog::updatePreview()
{
    m_preview->setThemeColors(currentColors());
}

void CustomThemeDialog::newTheme()
{
    m_editingName.clear();
    m_nameEdit->clear();
    m_nameEdit->setFocus();
    m_isDarkCheck->setChecked(true);
    for (auto it = m_colorSwatches.begin(); it != m_colorSwatches.end(); ++it)
    {
        QPushButton* swatch = it.value();
        swatch->setStyleSheet(QString("background-color: #2d2d2d; border: 1px solid gray; border-radius: 3px;"));
    }
    updatePreview();
    m_saveApplyBtn->setEnabled(true);
}

void CustomThemeDialog::deleteTheme()
{
    QListWidgetItem* item = m_themeList->currentItem();
    if (!item)
        return;
    QString name = item->text();

    int ret = QMessageBox::question(this, tr("Delete Theme"), tr("Delete the custom theme “%1”? This cannot be undone.").arg(name),
                                    QMessageBox::Yes | QMessageBox::No, QMessageBox::No);
    if (ret != QMessageBox::Yes)
        return;

    QString filePath = m_themesDir + QStringLiteral("/") + themeFileName(name);
    QFile::remove(filePath);

    // If this was the active custom theme, clear it.
    if (SettingsManager::instance().customTheme() == name)
        SettingsManager::instance().setCustomTheme(QString());

    refreshThemeList();
    m_editingName.clear();
    m_saveApplyBtn->setEnabled(false);
    m_deleteBtn->setEnabled(false);
}

QString CustomThemeDialog::themeFileName(const QString& name)
{
    QString safe = name;
    safe.replace(QRegularExpression(QStringLiteral("[^a-zA-Z0-9_.-]")), QStringLiteral("_"));
    if (safe.isEmpty())
        safe = QStringLiteral("custom");
    return safe + QStringLiteral(".json");
}

bool CustomThemeDialog::writeThemeJson(const ThemeColors& colors, const QString& filePath) const
{
    QFile file(filePath);
    if (!file.open(QFile::WriteOnly))
        return false;

    QJsonObject colorsObj;
    colorsObj[QStringLiteral("background")] = colorToJson(colors.background);
    colorsObj[QStringLiteral("foreground")] = colorToJson(colors.foreground);
    colorsObj[QStringLiteral("surfaceBg")] = colorToJson(colors.surfaceBg);
    colorsObj[QStringLiteral("surfaceFg")] = colorToJson(colors.surfaceFg);
    colorsObj[QStringLiteral("accent")] = colorToJson(colors.accent);
    colorsObj[QStringLiteral("border")] = colorToJson(colors.border);
    colorsObj[QStringLiteral("caret")] = colorToJson(colors.caret);
    colorsObj[QStringLiteral("lineHighlight")] = colorToJson(colors.lineHighlight);
    colorsObj[QStringLiteral("marginBg")] = colorToJson(colors.marginBg);
    colorsObj[QStringLiteral("marginFg")] = colorToJson(colors.marginFg);
    colorsObj[QStringLiteral("foldMarginBg")] = colorToJson(colors.foldMarginBg);
    colorsObj[QStringLiteral("foldMarginBgAlt")] = colorToJson(colors.foldMarginBgAlt);
    colorsObj[QStringLiteral("comment")] = colorToJson(colors.comment);
    colorsObj[QStringLiteral("keyword")] = colorToJson(colors.keyword);
    colorsObj[QStringLiteral("string")] = colorToJson(colors.string);
    colorsObj[QStringLiteral("number")] = colorToJson(colors.number);
    colorsObj[QStringLiteral("operator")] = colorToJson(colors.operator_);
    colorsObj[QStringLiteral("function")] = colorToJson(colors.function);
    colorsObj[QStringLiteral("preprocessor")] = colorToJson(colors.preprocessor);

    QJsonObject root;
    root[QStringLiteral("name")] = colors.name;
    root[QStringLiteral("isDark")] = colors.isDark;
    root[QStringLiteral("colors")] = colorsObj;

    file.write(QJsonDocument(root).toJson(QJsonDocument::Indented));
    return true;
}

bool CustomThemeDialog::saveTheme(bool applyAfter)
{
    ThemeColors colors = currentColors();
    if (colors.name.isEmpty())
    {
        QMessageBox::warning(this, tr("Invalid Theme"), tr("Please enter a theme name."));
        return false;
    }

    QDir dir(m_themesDir);
    if (!dir.exists() && !dir.mkpath(QStringLiteral(".")))
    {
        QMessageBox::critical(this, tr("Save Error"), tr("Could not create themes directory."));
        return false;
    }

    // Renaming: if editing an existing theme under a new name, remove the old file.
    if (!m_editingName.isEmpty() && m_editingName != colors.name)
    {
        QString oldPath = m_themesDir + QStringLiteral("/") + themeFileName(m_editingName);
        QFile::remove(oldPath);
    }

    QString filePath = m_themesDir + QStringLiteral("/") + themeFileName(colors.name);
    if (!writeThemeJson(colors, filePath))
    {
        QMessageBox::critical(this, tr("Save Error"), tr("Could not write theme file %1.").arg(filePath));
        return false;
    }

    // Refresh the in-memory cache so the new/edited theme is discoverable.
    loadCustomThemes();
    refreshThemeList();

    // Select the saved theme in the list.
    for (int i = 0; i < m_themeList->count(); ++i)
    {
        if (m_themeList->item(i)->text() == colors.name)
        {
            m_themeList->setCurrentRow(i);
            break;
        }
    }

    // If we renamed the active custom theme, keep the settings pointer valid.
    QString previousName = m_editingName;
    m_editingName = colors.name;
    m_saveApplyBtn->setEnabled(true);

    if (!previousName.isEmpty() && previousName != colors.name && SettingsManager::instance().customTheme() == previousName)
    {
        SettingsManager::instance().setCustomTheme(colors.name);
    }

    if (applyAfter)
    {
        SettingsManager::instance().setCustomTheme(colors.name);
        emit themeApplied(colors.name);
    }

    return true;
}
