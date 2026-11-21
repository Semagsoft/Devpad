/*
 * Devpad - Go language plugin
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
 * QScintilla has no native Go lexer; the C++ lexer is the closest match.
 */

#include "keywords.h"
#include "plugins/languagepluginbase.h"

#include <Qsci/qscilexercpp.h>

namespace
{
LanguagePluginDescriptor makeDescriptor()
{
    LanguagePluginDescriptor d;
    d.id = QStringLiteral("org.semagsoft.devpad.golanguage");
    d.name = QStringLiteral("Go Language");
    d.version = QStringLiteral("1.0.0");
    d.author = QStringLiteral("Semagsoft");
    d.description = QStringLiteral("Adds Go language support (keywords, .go files) via the plugin language hooks.");
    d.library = QStringLiteral("libgolanguage.so");
    d.primaryLanguageId = QStringLiteral("go");
    d.extensions = {QStringLiteral("go")};
    d.lineComment = QStringLiteral("//");
    d.blockCommentStart = QStringLiteral("/*");
    d.blockCommentEnd = QStringLiteral("*/");

    const auto factory = [](QObject* parent) -> QObject* { return new QsciLexerCPP(parent); };
    d.languages.append({QStringLiteral("go"), QStringLiteral("QsciLexerCPP"), factory, goKeywords()});
    return d;
}
} // namespace

extern "C" IPlugin* devpadCreatePlugin()
{
    return new LanguagePluginBase(makeDescriptor());
}
