/*
 * Devpad - Example "TOML Language" plugin
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
 * This plugin demonstrates the Devpad plugin participation hooks:
 *   - registerLanguage()  : contribute a new language/lexer
 *   - registerLanguageKeywords(): contribute keywords
 *   - registerExtension() : map .toml extension to the language
 *   - registerEncoding()  : contribute a custom encoding
 *
 * Install: copy build/libtomllanguage.so + tomllanguage.plugin.json to
 *   ~/.local/share/devpad/plugins/  (Linux/macOS)
 *   or %APDP%\devpad\plugins\     (Windows)
 *   Then restart Devpad and open a .toml file.
 */

#include "plugins/plugininterface.h"

#include <QObject>
#include <QStringList>

class TomlLanguagePlugin : public QObject, public IPlugin
{
    Q_OBJECT

public:
    explicit TomlLanguagePlugin(QObject* parent = nullptr) : QObject(parent) {}
    ~TomlLanguagePlugin() override = default;

    PluginInfo info() const override
    {
        PluginInfo i;
        i.id = QStringLiteral("org.semagsoft.devpad.tomllanguage");
        i.name = QStringLiteral("TOML Language");
        i.version = QStringLiteral("1.0.0");
        i.author = QStringLiteral("Semagsoft");
        i.description = QStringLiteral("Adds TOML language support via plugin participation hooks.");
        i.apiVersion = 1; // must match PluginManager::HostApiVersion
        i.library = QStringLiteral("libtomllanguage.so");
        i.enabled = true;
        return i;
    }

    bool initialize(IPluginHost* host) override
    {
        m_host = host;

        // Register the TOML language. A real plugin would return a QsciLexer*
        // from the factory (cast to QObject*); here we return nullptr which
        // means the language is recognized but has no syntax highlighting yet.
        auto factory = [](QObject* /*parent*/) -> QObject*
        {
            return nullptr;
        };
        if (!m_host->registerLanguage(QStringLiteral("toml"),
                                      QStringLiteral("QsciLexerCPP"),
                                      factory))
        {
            return false;
        }

        // Register TOML keywords.
        QStringList tomlKeywords = {
            "true", "false",
        };
        if (!m_host->registerLanguageKeywords(QStringLiteral("toml"), tomlKeywords))
        {
            return false;
        }

        // Map .toml extension to the "toml" language.
        if (!m_host->registerExtension(QStringLiteral("toml"), QStringLiteral("toml")))
        {
            return false;
        }

        // Map exact filename to the language.
        if (!m_host->registerFileNameMapping(QStringLiteral("CMakeLists.toml"), QStringLiteral("toml")))
        {
            return false;
        }

        // Register a custom encoding (e.g. a legacy Latin-1 code page).
        // Note: Qt6 does not ship a Windows-1252 codec, so the example
        // registers ISO-8859-1 (Latin-1), which IS supported.
        if (!m_host->registerEncoding(QStringLiteral("ISO-8859-1 (legacy)"),
                                      QStringLiteral("ISO-8859-1")))
        {
            return false;
        }

        return true;
    }

    void shutdown() override
    {
        m_host = nullptr;
    }

private:
    IPluginHost* m_host = nullptr;
};

// Exported C factory function resolved by the host via QLibrary::resolve().
extern "C" IPlugin* devpadCreatePlugin()
{
    return new TomlLanguagePlugin;
}

#include "tomllanguage.moc"
