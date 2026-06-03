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
#ifndef LANGUAGEINFO_H
#define LANGUAGEINFO_H

#include <QHash>
#include <QString>
#include <QStringList>
#include <functional>

#include <Qsci/qsciscintilla.h>

struct ThemeColors;

using LexerFactory = std::function<QsciLexer*(QObject* parent)>;
using KeywordProvider = std::function<const QStringList&()>;
using ThemeApplicator = std::function<void(QsciLexer*, const ThemeColors&)>;

struct LanguageInfo
{
    QString name;
    QString lexerClassName;
    LexerFactory factory;
    KeywordProvider keywords;
    ThemeApplicator themeApplicator;
};

const ThemeApplicator& cppThemeApplicator();
const ThemeApplicator& pythonThemeApplicator();
const ThemeApplicator& htmlThemeApplicator();
const ThemeApplicator& javascriptThemeApplicator();
const ThemeApplicator& csharpThemeApplicator();
const ThemeApplicator& javaThemeApplicator();
const ThemeApplicator& cssThemeApplicator();
const ThemeApplicator& xmlThemeApplicator();
const ThemeApplicator& sqlThemeApplicator();
const ThemeApplicator& bashThemeApplicator();
const ThemeApplicator& cmakeThemeApplicator();
const ThemeApplicator& markdownThemeApplicator();

const QHash<QString, ThemeApplicator>& themeApplicatorCache();
const std::vector<LanguageInfo>& languageTable();
const LanguageInfo* findLanguage(const QString& name);

#endif // LANGUAGEINFO_H
