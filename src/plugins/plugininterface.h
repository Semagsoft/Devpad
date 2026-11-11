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
#include "theme.h"

#include <QObject>
#include <QString>
#include <QStringList>
#include <QVariant>
#include <functional>

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
    virtual QAction* registerAction(const QString& id, const QString& text, const QKeySequence& shortcut, std::function<void()> callback) = 0;
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
    virtual QVariant pluginSetting(const QString& pluginId, const QString& key, const QVariant& defaultValue = QVariant()) const = 0;
    virtual void setPluginSetting(const QString& pluginId, const QString& key, const QVariant& value) = 0;
    virtual void removePluginSetting(const QString& pluginId, const QString& key) = 0;
    virtual QStringList pluginKeys(const QString& pluginId) const = 0;

    // ── LSP ──────────────────────────────────────────────────────
    // Registers an external LSP server command for a language. The server is
    // started automatically when a file of that language is opened.
    virtual void registerLspServer(const QString& language, const QString& command, const QStringList& args) = 0;

    // ── Language / syntax ────────────────────────────────────────
    // Registers a custom language/lexer. factory creates a QsciLexer
    // (passed as QObject* so plugins don't need QScintilla headers).
    // Returns false if languageId is already registered.
    using LexerFactory = std::function<QObject*(QObject* parent)>;
    virtual bool registerLanguage(const QString& languageId, const QString& lexerClassName, LexerFactory factory) = 0;
    // Registers keyword list for a language (comma-separated keywords are
    // split automatically, as with built-in languages).
    virtual bool registerLanguageKeywords(const QString& languageId, const QStringList& keywords) = 0;

    // ── Encoding ─────────────────────────────────────────────────
    // Registers a custom text encoding. encodingName is passed to
    // QStringConverter (e.g. "ISO-8859-1", "Windows-1252"). Returns false if
    // displayName is already registered.
    virtual bool registerEncoding(const QString& displayName, const QString& encodingName) = 0;

    // ── File type mapping ────────────────────────────────────────
    // Maps a file extension (e.g. "rs") to a language id. Extension is stored
    // lowercased; leading dot is optional and stripped if present.
    virtual bool registerExtension(const QString& extension, const QString& languageId) = 0;
    // Maps an exact file name (e.g. "CMakeLists.txt") to a language id.
    // File name is stored lowercased.
    virtual bool registerFileNameMapping(const QString& fileName, const QString& languageId) = 0;

    // ── Snippets ─────────────────────────────────────────────────
    // Register a snippet contributed by a plugin. scope is a comma-separated
    // list of language ids (e.g. "cpp,python"), or empty for global snippets
    // available in all languages. Returns false if name is already registered.
    virtual bool registerSnippet(const QString& name, const QString& prefix, const QStringList& body, const QString& description,
                                 const QString& scope) = 0;
    // Remove a plugin-contributed snippet by name. Returns false if not found.
    virtual bool unregisterSnippet(const QString& name) = 0;

    // ── Themes ───────────────────────────────────────────────────
    // Register a custom theme contributed by a plugin. themeId is a plugin-
    // specific string id (e.g. "myplugin.mydark"). Returns false if already
    // registered.
    virtual bool registerTheme(const QString& themeId, const ThemeColors& colors) = 0;
    // Unregister a plugin-contributed theme. Returns false if not found.
    virtual bool unregisterTheme(const QString& themeId) = 0;

    // ── Frontend ─────────────────────────────────────────────────
    virtual bool isWidgetFrontend() const = 0;

    // ── Debug control ────────────────────────────────────────────
    virtual void startDebugging(const QString& sessionKey, const QString& language, const QString& command, const QStringList& args)
    {
    }
    virtual void stopDebugging(const QString& sessionKey)
    {
    }
    virtual void debugContinue(const QString& sessionKey)
    {
    }
    virtual void debugStepOver(const QString& sessionKey)
    {
    }
    virtual void debugStepInto(const QString& sessionKey)
    {
    }
    virtual void debugStepOut(const QString& sessionKey)
    {
    }
    virtual void debugRestart(const QString& sessionKey)
    {
    }

    // ── Panels contributed by plugins / host ─────────────────────
    // Registers a widget (e.g. a dock panel) keyed by id so it can be looked
    // up later by id. Used by the host to expose built-in panels to plugins,
    // and by plugins to expose their own panels to the host.
    virtual void registerPanel(const QString& id, QWidget* widget) = 0;
    virtual QWidget* panel(const QString& id) const = 0;

    // ── Generic object registry ──────────────────────────────────
    // Registers a QObject (e.g. a manager) keyed by id so it can be looked
    // up later by id. Used by plugins to expose objects to MainWindow.
    virtual void registerObject(const QString& id, QObject* object) = 0;
    virtual QObject* object(const QString& id) const = 0;
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
    virtual void onEditorCreated(CodeEditor* editor)
    {
        Q_UNUSED(editor);
    }
    virtual void onEditorClosed(CodeEditor* editor)
    {
        Q_UNUSED(editor);
    }
    virtual void onCurrentEditorChanged(CodeEditor* editor)
    {
        Q_UNUSED(editor);
    }
    virtual void onFileOpened(const QString& filePath)
    {
        Q_UNUSED(filePath);
    }
    virtual void onFileSaved(const QString& filePath)
    {
        Q_UNUSED(filePath);
    }
    virtual void onFileModifiedExternally(const QString& filePath)
    {
        Q_UNUSED(filePath);
    }
    virtual void onSettingsChanged()
    {
    }
};

#define DEVPAD_PLUGIN_IID "org.semagsoft.devpad.IPlugin/1.0"

/**
 * Plugin factory function signature exported by every plugin library.
 */
using DevpadCreatePluginFunc = IPlugin* (*)();

#endif // PLUGININTERFACE_H
