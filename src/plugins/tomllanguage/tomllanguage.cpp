/*
 * Devpad - TOML language plugin
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
 *
 * QScintilla has no native TOML lexer. The Properties lexer already styles
 * key = value assignments, [sections] and # comments, which covers the core
 * of TOML; theme colors for it live in languageinfo.cpp.
 */

#include "keywords.h"
#include "plugins/languagepluginbase.h"

namespace
{
LanguagePluginDescriptor makeDescriptor()
{
    LanguagePluginDescriptor d;
    d.id = QStringLiteral("org.semagsoft.devpad.tomllanguage");
    d.name = QStringLiteral("TOML Language");
    d.version = QStringLiteral("1.0.0");
    d.author = QStringLiteral("Semagsoft");
    d.description = QStringLiteral("Adds TOML syntax highlighting, keywords and file mappings via the plugin language hooks.");
    d.library = QStringLiteral("libtomllanguage.so");
    d.primaryLanguageId = QStringLiteral("toml");
    d.extensions = {QStringLiteral("toml")};
    d.fileNames = {QStringLiteral("Cargo.toml"), QStringLiteral("pyproject.toml"), QStringLiteral("Pipfile"), QStringLiteral("Gopkg.toml")};
    d.lineComment = QStringLiteral("#");

    d.languages.append({QStringLiteral("toml"), QStringLiteral("QsciLexerProperties"), tomlKeywords()});
    return d;
}
} // namespace

extern "C" IPlugin* devpadCreatePlugin()
{
    return new LanguagePluginBase(makeDescriptor());
}
