/*
 * Devpad - JSONC language plugin
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
 * QScintilla has a JSON lexer but no JSONC (JSON with comments) lexer; the
 * JSON lexer already styles comment tokens, so it is reused here.
 */

#include "keywords.h"
#include "plugins/languagepluginbase.h"

#include <Qsci/qscilexerjson.h>

namespace
{
LanguagePluginDescriptor makeDescriptor()
{
    LanguagePluginDescriptor d;
    d.id = QStringLiteral("org.semagsoft.devpad.jsonclanguage");
    d.name = QStringLiteral("JSONC Language");
    d.version = QStringLiteral("1.0.0");
    d.author = QStringLiteral("Semagsoft");
    d.description = QStringLiteral("Adds JSON with comments support (.jsonc files) via the plugin language hooks.");
    d.library = QStringLiteral("libjsonclanguage.so");
    d.primaryLanguageId = QStringLiteral("jsonc");
    d.extensions = {QStringLiteral("jsonc")};
    d.lineComment = QStringLiteral("//");
    d.blockCommentStart = QStringLiteral("/*");
    d.blockCommentEnd = QStringLiteral("*/");

    const auto factory = [](QObject* parent) -> QObject* { return new QsciLexerJSON(parent); };
    d.languages.append({QStringLiteral("jsonc"), QStringLiteral("QsciLexerJSON"), factory, jsonKeywords()});
    return d;
}
} // namespace

extern "C" IPlugin* devpadCreatePlugin()
{
    return new LanguagePluginBase(makeDescriptor());
}
