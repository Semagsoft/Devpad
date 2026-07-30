#include "brackethelper.h"

#include <Qsci/qscilexer.h>
#include <Qsci/qsciscintilla.h>

#include <array>

BracketContext BracketHelper::contextAtPosition(const QsciScintilla* editor, int pos)
{
    BracketContext ctx;
    if (pos <= 0 || !editor)
        return ctx;

    int prevStyle = static_cast<int>(editor->SendScintilla(QsciScintilla::SCI_GETSTYLEAT, pos - 1));
    if (prevStyle == 0)
        return ctx;

    auto* lexer = editor->lexer();
    if (!lexer)
        return ctx;

    int line = editor->SendScintilla(QsciScintilla::SCI_LINEFROMPOSITION, pos);
    int lineStart = editor->SendScintilla(QsciScintilla::SCI_POSITIONFROMLINE, line);

    int commentStyleCount = 0;
    for (int s = 1; s < 32; ++s)
    {
        QString desc = lexer->description(s);
        if (desc.isEmpty())
            continue;
        if (commentStyleCount == 0 && desc.contains(QStringLiteral("Comment"), Qt::CaseInsensitive))
        {
            if (prevStyle == s)
            {
                ctx.inComment = true;
                return ctx;
            }
            ++commentStyleCount;
        }
        if (desc.contains(QStringLiteral("String"), Qt::CaseInsensitive))
        {
            if (prevStyle == s)
            {
                ctx.inString = true;
                return ctx;
            }
        }
    }

    int lineLength = editor->SendScintilla(QsciScintilla::SCI_GETLINEENDPOSITION, line);
    QString lineText = editor->text(lineStart, lineLength);
    int col = pos - lineStart;

    bool inBlockComment = false;
    for (int i = 0; i < col && i < lineText.length(); ++i)
    {
        QChar c = lineText[i];
        if (inBlockComment)
        {
            if (c == '*' && i + 1 < lineText.length() && lineText[i + 1] == '/')
            {
                inBlockComment = false;
                ++i;
            }
            continue;
        }
        if (ctx.inComment)
            continue;
        if (c == '/' && i + 1 < lineText.length())
        {
            if (lineText[i + 1] == '*')
            {
                inBlockComment = true;
                ++i;
                continue;
            }
            if (lineText[i + 1] == '/')
            {
                ctx.inComment = true;
                continue;
            }
        }
        if (c == '\\' && (ctx.inString || ctx.inCharLiteral))
        {
            ++i;
            continue;
        }
        if (c == '"' && !ctx.inCharLiteral)
            ctx.inString = !ctx.inString;
        else if (c == '\'' && !ctx.inString)
            ctx.inCharLiteral = !ctx.inCharLiteral;
    }

    ctx.inBlockComment = inBlockComment;
    return ctx;
}

bool BracketHelper::handleAutoClose(QsciScintilla* editor, QChar ch, int pos)
{
    struct Pair
    {
        QChar open;
        QChar close;
    };
    static constexpr std::array pairs{Pair{'(', ')'}, Pair{'[', ']'}, Pair{'{', '}'}, Pair{'"', '"'}, Pair{'\'', '\''}};
    BracketContext ctx = contextAtPosition(editor, pos);
    if (ctx.inComment || ctx.inBlockComment || ctx.inCharLiteral)
        return false;

    for (const auto& pair : pairs)
    {
        if (ch != pair.open)
            continue;
        if (ch == '"' && ctx.inString)
            continue;
        if (ch == '\'' && ctx.inCharLiteral)
            continue;
        editor->beginUndoAction();
        editor->insert(QString(pair.open) + pair.close);
        int l, c;
        editor->lineIndexFromPosition(pos + 1, &l, &c);
        editor->setCursorPosition(l, c);
        editor->endUndoAction();
        return true;
    }
    return false;
}

bool BracketHelper::handleBracketSkip(QsciScintilla* editor, QChar ch, int pos)
{
    static constexpr std::array closers{QChar(')'), QChar(']'), QChar('}'), QChar('"'), QChar('\'')};
    BracketContext ctx = contextAtPosition(editor, pos);

    if (ctx.inComment || ctx.inBlockComment)
        return false;

    bool isQuote = (ch == '"' || ch == '\'');
    if (isQuote)
    {
        bool inRelevantString = (ch == '"' && ctx.inString) || (ch == '\'' && ctx.inCharLiteral);
        if (!inRelevantString)
            return false;
    }
    else
    {
        if (ctx.inString || ctx.inCharLiteral)
            return false;
    }

    for (QChar closer : closers)
    {
        if (ch != closer)
            continue;
        int nextPos = pos + 1;
        if (nextPos >= editor->length())
            return false;
        int nextChar = editor->SendScintilla(QsciScintilla::SCI_GETCHARAT, nextPos);
        if (nextChar == static_cast<int>(closer.toLatin1()))
        {
            int nl, nc;
            editor->lineIndexFromPosition(nextPos, &nl, &nc);
            editor->setCursorPosition(nl, nc);
            return true;
        }
    }
    return false;
}
