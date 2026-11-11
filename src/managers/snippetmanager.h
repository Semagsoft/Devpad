#ifndef SNIPPETMANAGER_H
#define SNIPPETMANAGER_H

#include "snippet.h"

#include <QHash>
#include <QList>
#include <QObject>
#include <QString>

class SnippetManager : public QObject
{
    Q_OBJECT

public:
    explicit SnippetManager(QObject* parent = nullptr);
    ~SnippetManager() override;

    QList<Snippet> snippetsForLanguage(const QString& language) const;
    QList<Snippet> allSnippets() const;

    const QHash<QString, QList<Snippet>>& snippetMap() const
    {
        return m_snippets;
    }

    Snippet snippetByName(const QString& name) const;
    QList<Snippet> snippetsByPrefix(const QString& prefix, const QString& language = QString()) const;

    void loadFromJson(const QByteArray& jsonData);

    // ── Plugin snippet registration ──────────────────────────────
    // Register a snippet contributed by a plugin. scope is a comma-separated
    // list of language ids (e.g. "cpp,python"), or empty for global snippets
    // available in all languages. Returns false if name is already registered.
    bool addSnippet(const QString& name, const QString& prefix, const QStringList& body, const QString& description, const QString& scope);
    // Remove a plugin-contributed snippet by name. Returns false if not found.
    bool removeSnippet(const QString& name);

    static SnippetManager* instance();
    static void initialize(QObject* parent = nullptr);

private:
    void loadBuiltIn();
    void loadUserSnippets();

    QHash<QString, QList<Snippet>> m_snippets; // keyed by language
    QList<Snippet> m_globalSnippets;           // snippets with no scope

    static SnippetManager* s_instance;
};

#endif // SNIPPETMANAGER_H
