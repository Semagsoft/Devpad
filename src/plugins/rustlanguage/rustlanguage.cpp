/*
 * Devpad - Rust language plugin
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
 * QScintilla has no native Rust lexer; the C++ lexer is the closest match.
 */

#include "keywords.h"
#include "plugins/languagepluginbase.h"

#include <Qsci/qscilexercpp.h>

namespace
{
LanguagePluginDescriptor makeDescriptor()
{
    LanguagePluginDescriptor d;
    d.id = QStringLiteral("org.semagsoft.devpad.rustlanguage");
    d.name = QStringLiteral("Rust Language");
    d.version = QStringLiteral("1.0.0");
    d.author = QStringLiteral("Semagsoft");
    d.description = QStringLiteral("Adds Rust language support (keywords, .rs files) via the plugin language hooks.");
    d.library = QStringLiteral("librustlanguage.so");
    d.primaryLanguageId = QStringLiteral("rust");
    d.extensions = {QStringLiteral("rs")};
    d.lineComment = QStringLiteral("//");
    d.blockCommentStart = QStringLiteral("/*");
    d.blockCommentEnd = QStringLiteral("*/");

    const auto factory = [](QObject* parent) -> QObject* { return new QsciLexerCPP(parent); };
    const QStringList keywords = rustKeywords();
    d.languages.append({QStringLiteral("rust"), QStringLiteral("QsciLexerCPP"), factory, keywords});
    d.languages.append({QStringLiteral("rs"), QStringLiteral("QsciLexerCPP"), factory, keywords});
    return d;
}
} // namespace

extern "C" IPlugin* devpadCreatePlugin()
{
    return new LanguagePluginBase(makeDescriptor());
}
