#ifndef BRACKETHELPER_H
#define BRACKETHELPER_H

#include <QChar>

class QsciScintilla;

struct BracketContext
{
    bool inString = false;
    bool inCharLiteral = false;
    bool inComment = false;
    bool inBlockComment = false;
};

class BracketHelper
{
public:
    static BracketContext contextAtPosition(const QsciScintilla* editor, int pos);
    static bool handleAutoClose(QsciScintilla* editor, QChar ch, int pos);
    static bool handleBracketSkip(QsciScintilla* editor, QChar ch, int pos);
};

#endif // BRACKETHELPER_H
