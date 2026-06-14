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

ThemeApplier::ThemeApplier(QObject *parent) : QObject(parent) {}

void ThemeApplier::applyTheme(QMainWindow *window) {
    ThemeId themeId = SettingsManager::instance().theme();
    ThemeColors colors = getThemeColors(themeId);

    if (!prefersNativeStyling(themeId)) {
        QPalette palette = getThemePalette(colors);
        QApplication::setPalette(palette);
    }

    const auto& ui = SettingsManager::instance().uiSettings();
    if (!ui.uiFontFamily.isEmpty()) {
        QFont uiFont(ui.uiFontFamily, ui.uiFontSize);
        QApplication::setFont(uiFont);
    }

    if (!prefersNativeStyling(themeId)) {
        QFile qssFile(":/theme.qss");
        if (qssFile.open(QFile::ReadOnly | QFile::Text)) {
            QString ss = QString::fromUtf8(qssFile.readAll());

            struct { const char *key; QColor ThemeColors::*member; } const mappings[] = {
                {"%background%",           &ThemeColors::background},
                {"%foreground%",           &ThemeColors::foreground},
                {"%menuBg%",               &ThemeColors::menuBg},
                {"%menuFg%",               &ThemeColors::menuFg},
                {"%separator%",            &ThemeColors::separator},
                {"%selectionBg%",          &ThemeColors::selectionBg},
                {"%toolbarBg%",            &ThemeColors::toolbarBg},
                {"%toolbarFg%",            &ThemeColors::toolbarFg},
                {"%tabBg%",                &ThemeColors::tabBg},
                {"%tabFg%",                &ThemeColors::tabFg},
                {"%tabBgActive%",          &ThemeColors::tabBgActive},
                {"%tabFgActive%",          &ThemeColors::tabFgActive},
                {"%statusbarBg%",          &ThemeColors::statusbarBg},
                {"%statusbarFg%",          &ThemeColors::statusbarFg},
                {"%groupboxBg%",           &ThemeColors::groupboxBg},
                {"%groupboxFg%",           &ThemeColors::groupboxFg},
                {"%inputBg%",              &ThemeColors::inputBg},
                {"%inputFg%",              &ThemeColors::inputFg},
                {"%checkboxIndicator%",    &ThemeColors::checkboxIndicator},
                {"%buttonBg%",             &ThemeColors::buttonBg},
                {"%buttonFg%",             &ThemeColors::buttonFg},
                {"%scrollbarBg%",          &ThemeColors::scrollbarBg},
                {"%scrollbarHandle%",      &ThemeColors::scrollbarHandle},
                {"%scrollbarHandleHover%", &ThemeColors::scrollbarHandleHover},
            };
            for (const auto& m : mappings) {
                ss.replace(QLatin1String(m.key), (colors.*(m.member)).name());
            }

            window->setStyleSheet(ss);
        }
    } else {
        window->setStyleSheet(QString());
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
