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

#ifndef LSPPLUGIN_H
#define LSPPLUGIN_H

#include "plugins/lspinterface.h"
#include "plugins/plugininterface.h"

#include <QHash>
#include <QList>
#include <QPointer>
#include <QString>
#include <functional>

namespace lsp
{

class LspServerManager;
class LspEditorSession;

/**
 * QObject implementation of ILspService. Owns the LspServerManager and the
 * live registry of attached editor bridges. Lives and dies with the plugin.
 */
class LspService : public ILspService
{
    Q_OBJECT

public:
    explicit LspService(QObject* parent = nullptr);
    ~LspService() override;

    void setNavigateCallback(std::function<void(const QString& filePath, int line, int column)> callback) override;

    // Document lifecycle
    void openDocument(const QString& language, const QString& uri, const QString& text) override;
    void changeDocument(const QString& uri, const QString& text) override;
    void closeDocument(const QString& uri) override;
    void saveDocument(const QString& uri) override;

    // Sessions
    ILspEditorSession* attachEditor(IEditorLspBridge* bridge) override;
    void detachEditor(IEditorLspBridge* bridge) override;

    // Servers
    void registerServer(const QString& language, const QString& command, const QStringList& args) override;
    void unregisterServer(const QString& language) override;
    QStringList languages() const override;
    void requestWorkspaceSymbols(const QString& query) override;

private:
    LspServerManager* m_manager = nullptr;
    QList<QPointer<IEditorLspBridge>> m_bridges;
    QHash<IEditorLspBridge*, LspEditorSession*> m_sessions;
    std::function<void(const QString& filePath, int line, int column)> m_navigate;
};

class LspPlugin : public QObject, public IPlugin
{
    Q_OBJECT

public:
    explicit LspPlugin(QObject* parent = nullptr);
    ~LspPlugin() override;

    PluginInfo info() const override;
    bool initialize(IPluginHost* host) override;
    void shutdown() override;

private:
    IPluginHost* m_host = nullptr;
    LspService* m_service = nullptr;
};

} // namespace lsp

#endif // LSPPLUGIN_H
