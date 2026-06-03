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
#include "themeapplier.h"
#include "settingsmanager.h"
#include "tabmanager.h"
#include "codeeditor.h"
#include <QApplication>
#include <QFile>
#include <QMainWindow>
#include <QHash>

ThemeApplier::ThemeApplier(QObject *parent) : QObject(parent) {}

void ThemeApplier::applyTheme(QMainWindow *window) {
    ThemeId themeId = SettingsManager::instance().theme();
    ThemeColors colors = getThemeColors(themeId);
    QPalette palette = getThemePalette(colors);

    QApplication::setPalette(palette);

    const auto& ui = SettingsManager::instance().uiSettings();
    if (!ui.uiFontFamily.isEmpty()) {
        QFont uiFont(ui.uiFontFamily, ui.uiFontSize);
        QApplication::setFont(uiFont);
    }

    QFile qssFile(":/theme.qss");
    if (qssFile.open(QFile::ReadOnly | QFile::Text)) {
        QString ss = QString::fromUtf8(qssFile.readAll());

        QHash<QString, QString> vars;
        vars["%background%"] = colors.background.name();
        vars["%foreground%"] = colors.foreground.name();
        vars["%menuBg%"] = colors.menuBg.name();
        vars["%menuFg%"] = colors.menuFg.name();
        vars["%separator%"] = colors.separator.name();
        vars["%selectionBg%"] = colors.selectionBg.name();
        vars["%toolbarBg%"] = colors.toolbarBg.name();
        vars["%toolbarFg%"] = colors.toolbarFg.name();
        vars["%tabBg%"] = colors.tabBg.name();
        vars["%tabFg%"] = colors.tabFg.name();
        vars["%tabBgActive%"] = colors.tabBgActive.name();
        vars["%tabFgActive%"] = colors.tabFgActive.name();
        vars["%statusbarBg%"] = colors.statusbarBg.name();
        vars["%statusbarFg%"] = colors.statusbarFg.name();
        vars["%groupboxBg%"] = colors.groupboxBg.name();
        vars["%groupboxFg%"] = colors.groupboxFg.name();
        vars["%inputBg%"] = colors.inputBg.name();
        vars["%inputFg%"] = colors.inputFg.name();
        vars["%checkboxIndicator%"] = colors.checkboxIndicator.name();
        vars["%buttonBg%"] = colors.buttonBg.name();
        vars["%buttonFg%"] = colors.buttonFg.name();
        vars["%scrollbarBg%"] = colors.scrollbarBg.name();
        vars["%scrollbarHandle%"] = colors.scrollbarHandle.name();
        vars["%scrollbarHandleHover%"] = colors.scrollbarHandleHover.name();

        for (auto it = vars.constBegin(); it != vars.constEnd(); ++it) {
            ss.replace(it.key(), it.value());
        }

        window->setStyleSheet(ss);
    }
}

void ThemeApplier::applySettingsToAllEditors(TabManager *tabManager) {
    const auto& ed = SettingsManager::instance().editorSettings();

    tabManager->applySettingsToAll(ed);

    for (int i = 0; i < tabManager->count(); ++i) {
        CodeEditor *editor = tabManager->editorAt(i);
        if (editor) {
            editor->setWhitespaceVisible(ed.showWhitespace);
        }
    }
}
