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

#ifndef LANGUAGEPLUGINBASE_H
#define LANGUAGEPLUGINBASE_H

#include "plugins/plugininterface.h"

#include <QList>
#include <QString>
#include <QStringList>
#include <functional>
#include <utility>

/**
 * Descriptor for a single language contributed by a plugin. A plugin may
 * contribute several aliases (e.g. "rust" and "rs") that share a lexer and
 * keyword list, matching the historical built-in language table.
 */
struct LanguagePluginLanguage
{
    QString id;
    QString lexerClassName;
    std::function<QObject*(QObject*)> lexerFactory;
    QStringList keywords;
};

struct LanguagePluginDescriptor
{
    QString id;
    QString name;
    QString version;
    QString author;
    QString description;
    QString library;
    QList<LanguagePluginLanguage> languages;
    QString primaryLanguageId;
    QStringList extensions;
    QStringList fileNames;
    QString lineComment;
    QString blockCommentStart;
    QString blockCommentEnd;
};

/**
 * Reusable IPlugin implementation for languages that QScintilla does not ship
 * a dedicated lexer for. Each language plugin supplies only a descriptor;
 * registration, metadata and cleanup are handled here. PluginHost undoes all
 * registrations when the plugin is unloaded.
 */
class LanguagePluginBase : public IPlugin
{
public:
    explicit LanguagePluginBase(LanguagePluginDescriptor descriptor) : m_descriptor(std::move(descriptor))
    {
    }
    ~LanguagePluginBase() override = default;

    PluginInfo info() const override
    {
        PluginInfo i;
        i.id = m_descriptor.id;
        i.name = m_descriptor.name;
        i.version = m_descriptor.version;
        i.author = m_descriptor.author;
        i.description = m_descriptor.description;
        i.apiVersion = 1; // PluginManager::HostApiVersion
        i.library = m_descriptor.library;
        i.enabled = true;
        return i;
    }

    bool initialize(IPluginHost* host) override
    {
        m_host = host;
        for (const auto& language : m_descriptor.languages)
        {
            if (!host->registerLanguage(language.id, language.lexerClassName, language.lexerFactory))
                return false;
            if (!language.keywords.isEmpty() && !host->registerLanguageKeywords(language.id, language.keywords))
                return false;
            if (!host->registerLanguageCommentSyntax(language.id, m_descriptor.lineComment, m_descriptor.blockCommentStart,
                                                     m_descriptor.blockCommentEnd))
            {
                return false;
            }
        }
        for (const auto& extension : m_descriptor.extensions)
        {
            if (!host->registerExtension(extension, m_descriptor.primaryLanguageId))
                return false;
        }
        for (const auto& fileName : m_descriptor.fileNames)
        {
            if (!host->registerFileNameMapping(fileName, m_descriptor.primaryLanguageId))
                return false;
        }
        return true;
    }

    void shutdown() override
    {
        m_host = nullptr;
    }

private:
    LanguagePluginDescriptor m_descriptor;
    IPluginHost* m_host = nullptr;
};

#endif // LANGUAGEPLUGINBASE_H
