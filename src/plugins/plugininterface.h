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

#ifndef PLUGININTERFACE_H
#define PLUGININTERFACE_H

#include "plugininfo.h"

#include <QVariant>
#include <functional>
#include <QString>
#include <QStringList>

class QAction;
class QKeySequence;
class QMenu;
class QMenuBar;
class QWidget;

class CodeEditor;

namespace lsp
{
class LspServerManager;
}

/**
 * IPluginHost is the host-side surface exposed to plugins. Plugins receive a
 * PluginHost* in IPlugin::initialize() and use it to register actions, menu
 * items, dock panels, read/write their own settings, register LSP servers, and
 * observe editor/file events through the virtual handlers on IPlugin.
 *
 * This is a plain C++ interface (no Q_OBJECT) so that plugin libraries do not
 * need to link against Devpad_lib and do not depend on Devpad's meta-object
 * layout. PluginHost (in Devpad_lib) is the concrete QObject implementation.
 */
class IPluginHost
{
public:
    virtual ~IPluginHost() = default;

    // ── Core access (typed pointers, C++ plugins) ────────────────
    virtual QObject* mainWindow() const = 0;
    virtual QObject* actionManager() const = 0;
    virtual QObject* settingsManager() const = 0;
    virtual QObject* editorController() const = 0;
    virtual QObject* tabManager() const = 0;
    virtual QObject* lspServerManager() const = 0;

    // ── Actions / commands ───────────────────────────────────────
    // Registers a plugin-owned action. Returns the QAction* so the plugin can
    // place it in menus/toolbars itself. id must be unique within the plugin.
    virtual QAction* registerAction(const QString& id, const QString& text,
                                    const QKeySequence& shortcut,
                                    std::function<void()> callback) = 0;
    virtual bool unregisterAction(const QString& id) = 0;
    virtual QAction* action(const QString& id) const = 0;

    // ── Menus ────────────────────────────────────────────────────
    // Returns the top-level menu bar (widget frontend only).
    virtual QMenuBar* menuBar() const = 0;
    // Returns (creating if needed) a menu at the given path, e.g. "Tools" or
    // "Tools/Plugins". Returns nullptr when menus are unavailable (TUI).
    virtual QMenu* menu(const QString& path) const = 0;
    virtual void addMenuAction(const QString& menuPath, QAction* action) = 0;

    // ── Dock widgets / panels (widget frontend only) ─────────────
    // Adds a dock widget and returns a stable dock id the plugin can use to
    // remove it later. Returns empty string when docks are unavailable.
    virtual QString addDockWidget(QWidget* widget, Qt::DockWidgetArea area) = 0;
    virtual bool removeDockWidget(const QString& dockId) = 0;

    // ── Plugin-scoped settings ───────────────────────────────────
    // Read/write plugin-specific settings stored under Plugins/<id>/<key>.
    virtual QVariant pluginSetting(const QString& pluginId, const QString& key,
                                   const QVariant& defaultValue = QVariant()) const = 0;
    virtual void setPluginSetting(const QString& pluginId, const QString& key,
                                  const QVariant& value) = 0;
    virtual void removePluginSetting(const QString& pluginId, const QString& key) = 0;
    virtual QStringList pluginKeys(const QString& pluginId) const = 0;

    // ── LSP ──────────────────────────────────────────────────────
    // Registers an external LSP server command for a language. The server is
    // started automatically when a file of that language is opened.
    virtual void registerLspServer(const QString& language, const QString& command,
                                   const QStringList& args) = 0;

    // ── Frontend ─────────────────────────────────────────────────
    virtual bool isWidgetFrontend() const = 0;
};

/**
 * IPlugin is the plugin entry point. Each plugin library exports a C factory
 * function named devpadCreatePlugin() returning a new IPlugin instance.
 *
 * Plugins are loaded with QLibrary (which resolves exported symbols), so they
 * do not need Q_PLUGIN_METADATA and do not link against Devpad_lib.
 */
class IPlugin
{
public:
    virtual ~IPlugin() = default;

    virtual PluginInfo info() const = 0;
    /**
     * Called once after the plugin library is loaded. The host pointer remains
     * valid until shutdown() is called. Return false to abort loading.
     */
    virtual bool initialize(IPluginHost* host) = 0;
    /**
     * Called before the plugin library is unloaded. The plugin must release
     * all actions, dock widgets, and connections it created.
     */
    virtual void shutdown() = 0;

    // ── Editor / file event handlers (default: do nothing) ─────────
    virtual void onEditorCreated(CodeEditor* editor) { Q_UNUSED(editor); }
    virtual void onEditorClosed(CodeEditor* editor) { Q_UNUSED(editor); }
    virtual void onCurrentEditorChanged(CodeEditor* editor) { Q_UNUSED(editor); }
    virtual void onFileOpened(const QString& filePath) { Q_UNUSED(filePath); }
    virtual void onFileSaved(const QString& filePath) { Q_UNUSED(filePath); }
    virtual void onSettingsChanged() {}
};

#define DEVPAD_PLUGIN_IID "org.semagsoft.devpad.IPlugin/1.0"

/**
 * Plugin factory function signature exported by every plugin library.
 */
typedef IPlugin* (*DevpadCreatePluginFunc)();

#endif // PLUGININTERFACE_H
