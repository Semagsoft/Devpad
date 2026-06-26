#ifndef SNIPPET_H
#define SNIPPET_H

#include <QString>
#include <QStringList>
#include <QList>
#include <QPair>
#include <QMap>

struct Snippet
{
    QString name;
    QString prefix;
    QString description;
    QStringList body; // each element is a line
    QStringList scope;

    struct TabStop
    {
        int number = 0;
        QList<int> positions; // positions in expanded text
        QString defaultValue;
        int length = 0;
    };

    struct ExpandedSnippet
    {
        QString text;
        QList<TabStop> tabStops;
    };

    ExpandedSnippet expand() const;

    // Parse a VS Code-format snippet body string into our representation
    static ExpandedSnippet parseBody(const QStringList& bodyLines);
};

#endif // SNIPPET_H
