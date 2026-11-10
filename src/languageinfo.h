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

struct CommentSyntax
{
    QString lineComment;
    QString blockCommentStart;
    QString blockCommentEnd;
};

const CommentSyntax* commentSyntaxForLanguage(const QString& language);

struct LanguageInfo
{
    QString name;
    QString lexerClassName;
    LexerFactory factory;
    KeywordProvider keywords;
    ThemeApplicator themeApplicator;
    CommentSyntax commentSyntax;
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
const ThemeApplicator& jsonThemeApplicator();
const ThemeApplicator& bashThemeApplicator();
const ThemeApplicator& cmakeThemeApplicator();
const ThemeApplicator& luaThemeApplicator();
const ThemeApplicator& markdownThemeApplicator();
const ThemeApplicator& qmlThemeApplicator();
const ThemeApplicator& phpThemeApplicator();

const QHash<QString, ThemeApplicator>& themeApplicatorCache();
const std::vector<LanguageInfo>& languageTable();
const LanguageInfo* findLanguage(const QString& name);

// ── Plugin language registration ──────────────────────────────
// Allows plugins to contribute new languages/lexers. Registered languages are
// found by findLanguage() alongside the built-in table.
// factory creates a QsciLexer (passed as QObject* to avoid requiring plugins
// to link against QScintilla).
using PluginLexerFactory = std::function<QObject*(QObject* parent)>;
bool registerPluginLanguage(const QString& languageId, const QString& lexerClassName, PluginLexerFactory factory);
bool unregisterPluginLanguage(const QString& languageId);
bool registerPluginLanguageKeywords(const QString& languageId, const QStringList& keywords);

const CommentSyntax* commentSyntaxForLanguage(const QString& language);
#endif // LANGUAGEINFO_H
