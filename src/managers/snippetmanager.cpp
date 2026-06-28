#include "snippetmanager.h"

#include <QDir>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QStandardPaths>

SnippetManager* SnippetManager::s_instance = nullptr;

SnippetManager::SnippetManager(QObject* parent)
    : QObject(parent)
{
    s_instance = this;
    loadBuiltIn();
    loadUserSnippets();
}

SnippetManager::~SnippetManager()
{
    if (s_instance == this)
        s_instance = nullptr;
}

SnippetManager* SnippetManager::instance()
{
    return s_instance;
}

void SnippetManager::initialize(QObject* parent)
{
    if (!s_instance)
    {
        new SnippetManager(parent);
    }
}

static QStringList parseScope(const QString& scope)
{
    QStringList result;
    for (const QString& s : scope.split(',', Qt::SkipEmptyParts))
    {
        result.append(s.trimmed());
    }
    return result;
}

void SnippetManager::loadFromJson(const QByteArray& jsonData)
{
    QJsonParseError error;
    QJsonDocument doc = QJsonDocument::fromJson(jsonData, &error);
    if (error.error != QJsonParseError::NoError || !doc.isObject())
        return;

    QJsonObject root = doc.object();

    for (auto it = root.begin(); it != root.end(); ++it)
    {
        const QString& snippetName = it.key();
        QJsonObject obj = it.value().toObject();

        Snippet s;
        s.name = snippetName;
        s.prefix = obj.value("prefix").toString();

        if (s.prefix.isEmpty())
            continue;

        s.description = obj.value("description").toString(snippetName);

        // Body can be a string or array of strings
        QJsonValue bodyVal = obj.value("body");
        if (bodyVal.isString())
        {
            s.body = QStringList{bodyVal.toString()};
        }
        else if (bodyVal.isArray())
        {
            QJsonArray bodyArr = bodyVal.toArray();
            for (const auto& line : bodyArr)
            {
                s.body.append(line.toString());
            }
        }
        else
        {
            continue;
        }

        // Scope can be a string like "cpp,javascript"
        QString scopeVal = obj.value("scope").toString();
        s.scope = parseScope(scopeVal);

        if (s.scope.isEmpty())
        {
            m_globalSnippets.append(s);
        }
        else
        {
            for (const QString& lang : s.scope)
            {
                m_snippets[lang].append(s);
            }
        }
    }
}

QList<Snippet> SnippetManager::snippetsForLanguage(const QString& language) const
{
    QList<Snippet> result = m_globalSnippets;
    result.append(m_snippets.value(language));
    return result;
}

QList<Snippet> SnippetManager::allSnippets() const
{
    QList<Snippet> result = m_globalSnippets;
    for (auto it = m_snippets.begin(); it != m_snippets.end(); ++it)
    {
        result.append(it.value());
    }
    return result;
}

Snippet SnippetManager::snippetByName(const QString& name) const
{
    for (const Snippet& s : allSnippets())
    {
        if (s.name == name)
            return s;
    }
    return Snippet();
}

QList<Snippet> SnippetManager::snippetsByPrefix(const QString& prefix, const QString& language) const
{
    QList<Snippet> result;
    QList<Snippet> candidates;
    if (language.isEmpty())
        candidates = allSnippets();
    else
        candidates = snippetsForLanguage(language);

    for (const Snippet& s : candidates)
    {
        if (s.prefix == prefix || s.prefix.startsWith(prefix))
        {
            result.append(s);
        }
    }
    return result;
}

void SnippetManager::loadBuiltIn()
{
    QFile builtIn(":/snippets/default.json");
    if (builtIn.open(QIODevice::ReadOnly))
    {
        loadFromJson(builtIn.readAll());
    }
}

void SnippetManager::loadUserSnippets()
{
    QString configPath = QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation);
    QDir snippetsDir(configPath + "/snippets");
    if (!snippetsDir.exists())
        return;

    for (const QFileInfo& fi : snippetsDir.entryInfoList({"*.json"}, QDir::Files))
    {
        QFile file(fi.absoluteFilePath());
        if (file.open(QIODevice::ReadOnly))
        {
            loadFromJson(file.readAll());
        }
    }
}
