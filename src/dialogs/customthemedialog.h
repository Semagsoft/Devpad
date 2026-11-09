#ifndef CUSTOMTHEMEDIALOG_H
#define CUSTOMTHEMEDIALOG_H

#include "theme.h"

#include <QColor>
#include <QDialog>
#include <QHash>
#include <QListWidget>
#include <QMap>

class QDialogButtonBox;
class QLineEdit;
class QPushButton;
class QCheckBox;
class ThemePreviewWidget;

// Dialog to create and edit custom user themes stored as JSON files in
// AppConfigLocation/themes/*.json. The JSON shape matches parseThemeJson()
// so edited themes load back into the theme system unchanged.
//
// Emits themeApplied(name) when the user saves and asks for the new theme
// to become the active custom theme; consumers (e.g. MainWindow via the
// OptionsDialog) can then re-apply the theme live.
class CustomThemeDialog : public QDialog
{
    Q_OBJECT

public:
    explicit CustomThemeDialog(QWidget* parent = nullptr);
    ~CustomThemeDialog() override;

signals:
    void themeApplied(const QString& name);

private:
    QString m_themesDir;
    QString m_editingName; // empty when creating a new theme

    QListWidget* m_themeList = nullptr;
    QLineEdit* m_nameEdit = nullptr;
    QCheckBox* m_isDarkCheck = nullptr;
    ThemePreviewWidget* m_preview = nullptr;
    QDialogButtonBox* m_buttonBox = nullptr;
    QPushButton* m_saveBtn = nullptr;
    QPushButton* m_saveApplyBtn = nullptr;
    QPushButton* m_newBtn = nullptr;
    QPushButton* m_deleteBtn = nullptr;

    // color key -> swatch button
    QHash<QString, QPushButton*> m_colorSwatches;

    void setupUi();
    void refreshThemeList();
    void loadTheme(const QString& name);
    void newTheme();
    void deleteTheme();
    bool saveTheme(bool applyAfter);
    ThemeColors currentColors() const;
    void applyColorsToForm(const ThemeColors& colors);
    void pickColor(QPushButton* swatch);
    void updatePreview();
    bool writeThemeJson(const ThemeColors& colors, const QString& filePath) const;
    static QString themeFileName(const QString& name);
};

#endif // CUSTOMTHEMEDIALOG_H
