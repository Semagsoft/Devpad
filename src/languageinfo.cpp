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
#include "languageinfo.h"

#include "keywords.h"
#include "theme.h"

#include <Qsci/qscilexerbash.h>
#include <Qsci/qscilexercmake.h>
#include <Qsci/qscilexercpp.h>
#include <Qsci/qscilexercsharp.h>
#include <Qsci/qscilexercss.h>
#include <Qsci/qscilexerhtml.h>
#include <Qsci/qscilexerjava.h>
#include <Qsci/qscilexerjavascript.h>
#include <Qsci/qscilexermarkdown.h>
#include <Qsci/qscilexerpython.h>
#include <Qsci/qscilexersql.h>
#include <Qsci/qscilexerxml.h>

const ThemeApplicator& cppThemeApplicator()
{
    static const ThemeApplicator fn = [](QsciLexer* lexer, const ThemeColors& colors)
    {
        auto* l = static_cast<QsciLexerCPP*>(lexer);
        l->setColor(colors.comment, QsciLexerCPP::Comment);
        l->setColor(colors.comment, QsciLexerCPP::CommentLine);
        l->setColor(colors.comment, QsciLexerCPP::CommentDoc);
        l->setColor(colors.keyword, QsciLexerCPP::Keyword);
        l->setColor(colors.string, QsciLexerCPP::DoubleQuotedString);
        l->setColor(colors.string, QsciLexerCPP::SingleQuotedString);
        l->setColor(colors.number, QsciLexerCPP::Number);
        l->setColor(colors.preprocessor, QsciLexerCPP::PreProcessor);
        l->setColor(colors.operator_, QsciLexerCPP::Operator);
        l->setColor(colors.operator_, QsciLexerCPP::Default);
    };
    return fn;
}

const ThemeApplicator& pythonThemeApplicator()
{
    static const ThemeApplicator fn = [](QsciLexer* lexer, const ThemeColors& colors)
    {
        auto* l = static_cast<QsciLexerPython*>(lexer);
        l->setColor(colors.comment, QsciLexerPython::Comment);
        l->setColor(colors.keyword, QsciLexerPython::Keyword);
        l->setColor(colors.string, QsciLexerPython::DoubleQuotedString);
        l->setColor(colors.string, QsciLexerPython::SingleQuotedString);
        l->setColor(colors.number, QsciLexerPython::Number);
        l->setColor(colors.function, QsciLexerPython::FunctionMethodName);
        l->setColor(colors.operator_, QsciLexerPython::Default);
    };
    return fn;
}

const ThemeApplicator& htmlThemeApplicator()
{
    static const ThemeApplicator fn = [](QsciLexer* lexer, const ThemeColors& colors)
    {
        auto* l = static_cast<QsciLexerHTML*>(lexer);
        l->setColor(colors.comment, QsciLexerHTML::HTMLComment);
        l->setColor(colors.keyword, QsciLexerHTML::Tag);
        l->setColor(colors.string, QsciLexerHTML::HTMLDoubleQuotedString);
        l->setColor(colors.string, QsciLexerHTML::HTMLSingleQuotedString);
        l->setColor(colors.number, QsciLexerHTML::HTMLNumber);
        l->setColor(colors.operator_, QsciLexerHTML::Default);
    };
    return fn;
}

const ThemeApplicator& javascriptThemeApplicator()
{
    static const ThemeApplicator fn = [](QsciLexer* lexer, const ThemeColors& colors)
    {
        auto* l = static_cast<QsciLexerJavaScript*>(lexer);
        l->setColor(colors.comment, QsciLexerJavaScript::Comment);
        l->setColor(colors.comment, QsciLexerJavaScript::CommentLine);
        l->setColor(colors.keyword, QsciLexerJavaScript::Keyword);
        l->setColor(colors.string, QsciLexerJavaScript::DoubleQuotedString);
        l->setColor(colors.string, QsciLexerJavaScript::SingleQuotedString);
        l->setColor(colors.number, QsciLexerJavaScript::Number);
        l->setColor(colors.operator_, QsciLexerJavaScript::Default);
    };
    return fn;
}

const ThemeApplicator& csharpThemeApplicator()
{
    static const ThemeApplicator fn = [](QsciLexer* lexer, const ThemeColors& colors)
    {
        auto* l = static_cast<QsciLexerCSharp*>(lexer);
        l->setColor(colors.comment, QsciLexerCSharp::Comment);
        l->setColor(colors.comment, QsciLexerCSharp::CommentLine);
        l->setColor(colors.comment, QsciLexerCSharp::CommentDoc);
        l->setColor(colors.keyword, QsciLexerCSharp::Keyword);
        l->setColor(colors.string, QsciLexerCSharp::DoubleQuotedString);
        l->setColor(colors.string, QsciLexerCSharp::SingleQuotedString);
        l->setColor(colors.number, QsciLexerCSharp::Number);
        l->setColor(colors.preprocessor, QsciLexerCSharp::PreProcessor);
        l->setColor(colors.operator_, QsciLexerCSharp::Operator);
        l->setColor(colors.operator_, QsciLexerCSharp::Default);
    };
    return fn;
}

const ThemeApplicator& javaThemeApplicator()
{
    static const ThemeApplicator fn = [](QsciLexer* lexer, const ThemeColors& colors)
    {
        auto* l = static_cast<QsciLexerJava*>(lexer);
        l->setColor(colors.comment, QsciLexerJava::Comment);
        l->setColor(colors.comment, QsciLexerJava::CommentLine);
        l->setColor(colors.comment, QsciLexerJava::CommentDoc);
        l->setColor(colors.keyword, QsciLexerJava::Keyword);
        l->setColor(colors.string, QsciLexerJava::DoubleQuotedString);
        l->setColor(colors.string, QsciLexerJava::SingleQuotedString);
        l->setColor(colors.number, QsciLexerJava::Number);
        l->setColor(colors.operator_, QsciLexerJava::Operator);
        l->setColor(colors.operator_, QsciLexerJava::Default);
    };
    return fn;
}

const ThemeApplicator& cssThemeApplicator()
{
    static const ThemeApplicator fn = [](QsciLexer* lexer, const ThemeColors& colors)
    {
        auto* l = static_cast<QsciLexerCSS*>(lexer);
        l->setColor(colors.comment, QsciLexerCSS::Comment);
        l->setColor(colors.keyword, QsciLexerCSS::Tag);
        l->setColor(colors.string, QsciLexerCSS::DoubleQuotedString);
        l->setColor(colors.string, QsciLexerCSS::SingleQuotedString);
        l->setColor(colors.number, QsciLexerCSS::Value);
        l->setColor(colors.operator_, QsciLexerCSS::Default);
    };
    return fn;
}

const ThemeApplicator& xmlThemeApplicator()
{
    static const ThemeApplicator fn = [](QsciLexer* lexer, const ThemeColors& colors)
    {
        auto* l = static_cast<QsciLexerXML*>(lexer);
        l->setColor(colors.comment, QsciLexerXML::HTMLComment);
        l->setColor(colors.comment, QsciLexerXML::SGMLComment);
        l->setColor(colors.keyword, QsciLexerXML::Tag);
        l->setColor(colors.string, QsciLexerXML::HTMLDoubleQuotedString);
        l->setColor(colors.string, QsciLexerXML::HTMLSingleQuotedString);
        l->setColor(colors.string, QsciLexerXML::HTMLValue);
        l->setColor(colors.number, QsciLexerXML::HTMLNumber);
        l->setColor(colors.operator_, QsciLexerXML::Default);
    };
    return fn;
}

const ThemeApplicator& sqlThemeApplicator()
{
    static const ThemeApplicator fn = [](QsciLexer* lexer, const ThemeColors& colors)
    {
        auto* l = static_cast<QsciLexerSQL*>(lexer);
        l->setColor(colors.comment, QsciLexerSQL::Comment);
        l->setColor(colors.comment, QsciLexerSQL::CommentLine);
        l->setColor(colors.keyword, QsciLexerSQL::Keyword);
        l->setColor(colors.string, QsciLexerSQL::SingleQuotedString);
        l->setColor(colors.number, QsciLexerSQL::Number);
        l->setColor(colors.operator_, QsciLexerSQL::Default);
    };
    return fn;
}

const ThemeApplicator& bashThemeApplicator()
{
    static const ThemeApplicator fn = [](QsciLexer* lexer, const ThemeColors& colors)
    {
        auto* l = static_cast<QsciLexerBash*>(lexer);
        l->setColor(colors.foreground, QsciLexerBash::Default);
        l->setColor(colors.preprocessor, QsciLexerBash::Error);
        l->setColor(colors.comment, QsciLexerBash::Comment);
        l->setColor(colors.number, QsciLexerBash::Number);
        l->setColor(colors.keyword, QsciLexerBash::Keyword);
        l->setColor(colors.string, QsciLexerBash::DoubleQuotedString);
        l->setColor(colors.string, QsciLexerBash::SingleQuotedString);
        l->setColor(colors.operator_, QsciLexerBash::Operator);
        l->setColor(colors.foreground, QsciLexerBash::Identifier);
        l->setColor(colors.preprocessor, QsciLexerBash::Scalar);
        l->setColor(colors.preprocessor, QsciLexerBash::ParameterExpansion);
        l->setColor(colors.string, QsciLexerBash::Backticks);
        l->setColor(colors.string, QsciLexerBash::HereDocumentDelimiter);
        l->setColor(colors.string, QsciLexerBash::SingleQuotedHereDocument);
    };
    return fn;
}

const ThemeApplicator& cmakeThemeApplicator()
{
    static const ThemeApplicator fn = [](QsciLexer* lexer, const ThemeColors& colors)
    {
        auto* l = static_cast<QsciLexerCMake*>(lexer);
        l->setColor(colors.comment, QsciLexerCMake::Comment);
        l->setColor(colors.keyword, QsciLexerCMake::Function);
        l->setColor(colors.string, QsciLexerCMake::String);
        l->setColor(colors.number, QsciLexerCMake::Number);
        l->setColor(colors.operator_, QsciLexerCMake::Default);
    };
    return fn;
}

const ThemeApplicator& markdownThemeApplicator()
{
    static const ThemeApplicator fn = [](QsciLexer* lexer, const ThemeColors& colors)
    {
        auto* l = static_cast<QsciLexerMarkdown*>(lexer);
        l->setColor(colors.function, QsciLexerMarkdown::Header1);
        l->setColor(colors.function, QsciLexerMarkdown::Header2);
        l->setColor(colors.function, QsciLexerMarkdown::Header3);
        l->setColor(colors.function, QsciLexerMarkdown::Header4);
        l->setColor(colors.function, QsciLexerMarkdown::Header5);
        l->setColor(colors.function, QsciLexerMarkdown::Header6);
        l->setColor(colors.keyword, QsciLexerMarkdown::EmphasisAsterisks);
        l->setColor(colors.keyword, QsciLexerMarkdown::EmphasisUnderscores);
        l->setColor(colors.keyword, QsciLexerMarkdown::StrongEmphasisAsterisks);
        l->setColor(colors.keyword, QsciLexerMarkdown::StrongEmphasisUnderscores);
        l->setColor(colors.keyword, QsciLexerMarkdown::UnorderedListItem);
        l->setColor(colors.keyword, QsciLexerMarkdown::OrderedListItem);
        l->setColor(colors.comment, QsciLexerMarkdown::BlockQuote);
        l->setColor(colors.comment, QsciLexerMarkdown::StrikeOut);
        l->setColor(colors.operator_, QsciLexerMarkdown::HorizontalRule);
        l->setColor(colors.string, QsciLexerMarkdown::Link);
        l->setColor(colors.string, QsciLexerMarkdown::CodeBackticks);
        l->setColor(colors.string, QsciLexerMarkdown::CodeDoubleBackticks);
        l->setColor(colors.string, QsciLexerMarkdown::CodeBlock);
    };
    return fn;
}

const QHash<QString, ThemeApplicator>& themeApplicatorCache()
{
    static const QHash<QString, ThemeApplicator> cache = {
        {"QsciLexerCPP", cppThemeApplicator()},       {"QsciLexerPython", pythonThemeApplicator()},
        {"QsciLexerHTML", htmlThemeApplicator()},     {"QsciLexerJavaScript", javascriptThemeApplicator()},
        {"QsciLexerCSharp", csharpThemeApplicator()}, {"QsciLexerJava", javaThemeApplicator()},
        {"QsciLexerCSS", cssThemeApplicator()},       {"QsciLexerXML", xmlThemeApplicator()},
        {"QsciLexerSQL", sqlThemeApplicator()},       {"QsciLexerBash", bashThemeApplicator()},
        {"QsciLexerCMake", cmakeThemeApplicator()},   {"QsciLexerMarkdown", markdownThemeApplicator()},
    };
    return cache;
}

const std::vector<LanguageInfo>& languageTable()
{
    static const std::vector<LanguageInfo> table = {
        {"cpp", "QsciLexerCPP", []([[maybe_unused]] QObject* parent) -> QsciLexer* { return new QsciLexerCPP(parent); }, cppKeywords,
         cppThemeApplicator(), {"//", "/*", "*/"}},
        {"c", "QsciLexerCPP", []([[maybe_unused]] QObject* parent) -> QsciLexer* { return new QsciLexerCPP(parent); }, cppKeywords,
         cppThemeApplicator(), {"//", "/*", "*/"}},
        {"h", "QsciLexerCPP", []([[maybe_unused]] QObject* parent) -> QsciLexer* { return new QsciLexerCPP(parent); }, cppKeywords,
         cppThemeApplicator(), {"//", "/*", "*/"}},
        {"hpp", "QsciLexerCPP", []([[maybe_unused]] QObject* parent) -> QsciLexer* { return new QsciLexerCPP(parent); }, cppKeywords,
         cppThemeApplicator(), {"//", "/*", "*/"}},
        {"csharp", "QsciLexerCSharp", []([[maybe_unused]] QObject* parent) -> QsciLexer* { return new QsciLexerCSharp(parent); }, csharpKeywords,
         csharpThemeApplicator(), {"//", "/*", "*/"}},
        {"java", "QsciLexerJava", []([[maybe_unused]] QObject* parent) -> QsciLexer* { return new QsciLexerJava(parent); }, javaKeywords,
         javaThemeApplicator(), {"//", "/*", "*/"}},
        {"python", "QsciLexerPython", []([[maybe_unused]] QObject* parent) -> QsciLexer* { return new QsciLexerPython(parent); }, pythonKeywords,
         pythonThemeApplicator(), {"#",  "",   ""}},
        {"javascript", "QsciLexerJavaScript", []([[maybe_unused]] QObject* parent) -> QsciLexer* { return new QsciLexerJavaScript(parent); },
         javascriptKeywords, javascriptThemeApplicator(), {"//", "/*", "*/"}},
        {"html", "QsciLexerHTML", []([[maybe_unused]] QObject* parent) -> QsciLexer* { return new QsciLexerHTML(parent); }, htmlKeywords,
         htmlThemeApplicator(), {"",   "<!--", "-->"}},
        {"htm", "QsciLexerHTML", []([[maybe_unused]] QObject* parent) -> QsciLexer* { return new QsciLexerHTML(parent); }, htmlKeywords,
         htmlThemeApplicator(), {"",   "<!--", "-->"}},
        {"css", "QsciLexerCSS", []([[maybe_unused]] QObject* parent) -> QsciLexer* { return new QsciLexerCSS(parent); }, cssKeywords,
         cssThemeApplicator(), {"//", "/*", "*/"}},
        {"xml", "QsciLexerXML", []([[maybe_unused]] QObject* parent) -> QsciLexer* { return new QsciLexerXML(parent); }, xmlKeywords,
         xmlThemeApplicator(), {"",   "<!--", "-->"}},
        {"sql", "QsciLexerSQL", []([[maybe_unused]] QObject* parent) -> QsciLexer* { return new QsciLexerSQL(parent); }, sqlKeywords,
         sqlThemeApplicator(), {"--", "/*", "*/"}},
        {"typescript", "QsciLexerJavaScript", []([[maybe_unused]] QObject* parent) -> QsciLexer* { return new QsciLexerJavaScript(parent); },
         typescriptKeywords, javascriptThemeApplicator(), {"//", "/*", "*/"}},
        {"ts", "QsciLexerJavaScript", []([[maybe_unused]] QObject* parent) -> QsciLexer* { return new QsciLexerJavaScript(parent); },
         typescriptKeywords, javascriptThemeApplicator(), {"//", "/*", "*/"}},
        {"tsx", "QsciLexerJavaScript", []([[maybe_unused]] QObject* parent) -> QsciLexer* { return new QsciLexerJavaScript(parent); },
         typescriptKeywords, javascriptThemeApplicator(), {"//", "/*", "*/"}},
        {"rust", "QsciLexerCPP", []([[maybe_unused]] QObject* parent) -> QsciLexer* { return new QsciLexerCPP(parent); }, rustKeywords,
         cppThemeApplicator(), {"//", "/*", "*/"}},
        {"rs", "QsciLexerCPP", []([[maybe_unused]] QObject* parent) -> QsciLexer* { return new QsciLexerCPP(parent); }, rustKeywords,
         cppThemeApplicator(), {"//", "/*", "*/"}},
        {"go", "QsciLexerCPP", []([[maybe_unused]] QObject* parent) -> QsciLexer* { return new QsciLexerCPP(parent); }, goKeywords,
         cppThemeApplicator(), {"//", "/*", "*/"}},
        {"markdown", "QsciLexerMarkdown", []([[maybe_unused]] QObject* parent) -> QsciLexer* { return new QsciLexerMarkdown(parent); },
         markdownKeywords, markdownThemeApplicator(), {"",   "",   ""}},
        {"md", "QsciLexerMarkdown", []([[maybe_unused]] QObject* parent) -> QsciLexer* { return new QsciLexerMarkdown(parent); }, markdownKeywords,
         markdownThemeApplicator(), {"",   "",   ""}},
        {"cmake", "QsciLexerCMake", []([[maybe_unused]] QObject* parent) -> QsciLexer* { return new QsciLexerCMake(parent); }, cmakeKeywords,
         cmakeThemeApplicator(), {"#",  "#[[", "]]"}},
        {"bash", "QsciLexerBash", []([[maybe_unused]] QObject* parent) -> QsciLexer* { return new QsciLexerBash(parent); }, bashKeywords,
         bashThemeApplicator(), {"#",  "",   ""}},
    };
    return table;
}

const LanguageInfo* findLanguage(const QString& name)
{
    const auto& table = languageTable();
    for (const auto& lang : table)
    {
        if (lang.name == name)
        {
            return &lang;
        }
    }
    return nullptr;
}

const CommentSyntax* commentSyntaxForLanguage(const QString& language)
{
    const auto& table = languageTable();
    for (const auto& lang : table)
    {
        if (lang.name == language)
            return &lang.commentSyntax;
    }
    return nullptr;
}
