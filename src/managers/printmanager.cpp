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
#include "printmanager.h"

#include "codeeditor.h"

#include <QPrinter>
#include <QTextDocument>

#include <Qsci/qscilexer.h>

namespace
{
constexpr int SCI_GETSTYLEAT = 2008;
}

PrintManager::PrintManager(QObject* parent) : QObject(parent)
{
}

void PrintManager::printEditorWithHighlighting(CodeEditor* editor, QPrinter* printer)
{
    QTextDocument doc;
    doc.setDefaultFont(editor->font());

    QsciLexer* lexer = editor->lexer();
    if (!lexer)
    {
        doc.setPlainText(editor->text());
    }
    else
    {
        doc.setHtml(generateHighlightedHtml(editor));
    }

    doc.print(printer);
}

QString PrintManager::generateHighlightedHtml(CodeEditor* editor) const
{
    QString html;
    QString text = editor->text();
    int pos = 0;
    int length = text.length();

    QsciLexer* lexer = editor->lexer();
    QColor defaultColor = lexer->defaultColor();
    QColor defaultPaper = lexer->defaultPaper();
    QFont defaultFont = lexer->defaultFont();

    html += "<pre style=\"font-family: monospace;\">";

    while (pos < length)
    {
        int style = editor->SendScintilla(SCI_GETSTYLEAT, pos);
        QColor fgColor = lexer->color(style);
        if (!fgColor.isValid())
            fgColor = defaultColor;
        QColor bgColor = lexer->paper(style);
        if (!bgColor.isValid())
            bgColor = defaultPaper;
        QFont font = lexer->font(style);

        int endPos = pos + 1;
        while (endPos < length && editor->SendScintilla(SCI_GETSTYLEAT, endPos) == style)
        {
            endPos++;
        }

        QString segment = text.mid(pos, endPos - pos).toHtmlEscaped();
        segment.replace("\n", "<br>");
        segment.replace("  ", "&nbsp;&nbsp;");
        segment.replace("\t", "&nbsp;&nbsp;&nbsp;&nbsp;");

        QString styleAttr = QString("color: #%1; background-color: #%2;").arg(fgColor.name().mid(1)).arg(bgColor.name().mid(1));
        if (font.bold())
            styleAttr += " font-weight: bold;";
        if (font.italic())
            styleAttr += " font-style: italic;";

        html += QString("<span style=\"%1\">%2</span>").arg(styleAttr, segment);
        pos = endPos;
    }

    html += "</pre>";
    return html;
}
