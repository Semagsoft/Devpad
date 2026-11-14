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

#include "pluginhost.h"

#include "actionmanager.h"
#include "codeeditor.h"
#include "editorcontroller.h"
#include "encodingutils.h"
#include "languageinfo.h"
#include "lsp/lspservermanager.h"
#include "mainwindow.h"
#include "managers/filewatchermanager.h"
#include "managers/settingsmanager.h"
#include "managers/snippetmanager.h"
#include "managers/tabmanager.h"

#include <QAction>
#include <QMenu>
#include <QMenuBar>

PluginHost::PluginHost(MainWindow* mainWindow, QObject* parent) : QObject(parent), m_mainWindow(mainWindow)
{
    if (m_mainWindow)
    {
        m_actionManager = m_mainWindow->findChild<ActionManager*>();
        m_settingsManager = &SettingsManager::instance();
        m_editorController = m_mainWindow->findChild<EditorController*>();
        m_tabManager = m_mainWindow->findChild<TabManager*>();
        m_lspServerManager = m_mainWindow->findChild<lsp::LspServerManager*>();
        m_fileWatcherManager = m_mainWindow->findChild<FileWatcherManager*>();

        forwardEditorSignals();

        if (m_fileWatcherManager)
            connect(m_fileWatcherManager, &FileWatcherManager::fileModifiedExternally, this, &PluginHost::fileModifiedExternally);
    }
}

PluginHost::~PluginHost() = default;

QObject* PluginHost::mainWindow() const
{
    return m_mainWindow;
}

QObject* PluginHost::actionManager() const
{
    return m_actionManager;
}

QObject* PluginHost::settingsManager() const
{
    return m_settingsManager;
}

QObject* PluginHost::editorController() const
{
    return m_editorController;
}

QObject* PluginHost::tabManager() const
{
    return m_tabManager;
}

QObject* PluginHost::lspServerManager() const
{
    return m_lspServerManager;
}

void PluginHost::forwardEditorSignals()
{
    if (!m_tabManager)
        return;

    connect(m_tabManager, &TabManager::editorCreated, this, [this](CodeEditor* editor) { Q_EMIT editorCreated(editor); });
    connect(m_tabManager, &TabManager::editorClosed, this, [this](CodeEditor* editor) { Q_EMIT editorClosed(editor); });

    connect(m_tabManager, &TabManager::currentChanged, this,
            [this](int)
            {
                if (m_tabManager)
                    Q_EMIT currentEditorChanged(m_tabManager->currentEditor());
            });
}

void PluginHost::beginPluginSession(const QString& pluginId)
{
    m_currentPluginId = pluginId;
}

void PluginHost::endPluginSession(const QString& pluginId)
{
    // Clean up plugin registrations before removing other leftovers.
    auto regIt = m_pluginRegistrations.find(pluginId);
    if (regIt != m_pluginRegistrations.end())
    {
        const PluginRegistrations& reg = regIt.value();
        for (const auto& langId : reg.languageIds)
            unregisterPluginLanguage(langId);
        for (const auto& encName : reg.encodingNames)
            unregisterPluginEncoding(encName);
        for (const auto& ext : reg.extensions)
            SettingsManager::removeExtensionMapping(ext);
        for (const auto& fileName : reg.fileNames)
            SettingsManager::removeFileNameMapping(fileName);
        for (const auto& snippetName : reg.snippetNames)
            SnippetManager::instance().removeSnippet(snippetName);
        for (const auto& themeId : reg.themeIds)
            unregisterPluginTheme(themeId);
        m_pluginRegistrations.erase(regIt);
    }

    // Remove everything this plugin registered; must run while the plugin's
    // library is still loaded (deleteLater()ed widgets may belong to it).
    for (auto it = m_pluginActions.begin(); it != m_pluginActions.end();)
    {
        if (m_pluginActionOwners.value(it.key()) == pluginId)
        {
            if (QAction* act = it.value())
            {
                if (m_mainWindow)
                    m_mainWindow->removeAction(act);
                act->deleteLater();
            }
            it = m_pluginActions.erase(it);
        }
        else
        {
            ++it;
        }
    }
    m_pluginActionOwners.remove(pluginId);

    for (auto it = m_pluginDocks.begin(); it != m_pluginDocks.end();)
    {
        if (m_pluginDockOwners.value(it.key()) == pluginId)
        {
            if (DockEntry entry = it.value(); entry.dock)
            {
                m_mainWindow->removeDockWidget(entry.dock);
                entry.dock->deleteLater();
            }
            it = m_pluginDocks.erase(it);
        }
        else
        {
            ++it;
        }
    }
    m_pluginDockOwners.remove(pluginId);

    if (m_currentPluginId == pluginId)
        m_currentPluginId.clear();
}

QAction* PluginHost::registerAction(const QString& id, const QString& text, const QKeySequence& shortcut, std::function<void()> callback)
{
    if (!m_actionManager || id.isEmpty())
        return nullptr;
    if (m_pluginActions.contains(id))
        return m_pluginActions.value(id);

    auto* act = new QAction(text, m_actionManager);
    act->setShortcut(shortcut);
    act->setEnabled(true);
    connect(act, &QAction::triggered, this,
            [callback]
            {
                if (callback)
                    callback();
            });
    m_pluginActions.insert(id, act);
    m_pluginActionOwners.insert(id, m_currentPluginId);

    // Register the shortcut with the main window so it is active globally.
    if (m_mainWindow && !shortcut.isEmpty())
        m_mainWindow->addAction(act);

    return act;
}

bool PluginHost::unregisterAction(const QString& id)
{
    if (!m_actionManager || id.isEmpty())
        return false;

    QAction* act = m_pluginActions.take(id);
    if (!act)
        return false;

    if (m_mainWindow)
        m_mainWindow->removeAction(act);
    act->deleteLater();
    return true;
}

QAction* PluginHost::action(const QString& id) const
{
    return m_pluginActions.value(id, nullptr);
}

QMenuBar* PluginHost::menuBar() const
{
    return m_mainWindow ? m_mainWindow->menuBarWidget() : nullptr;
}

QMenu* PluginHost::menu(const QString& path) const
{
    if (!m_mainWindow || path.isEmpty())
        return nullptr;

    QMenuBar* bar = m_mainWindow->menuBarWidget();
    if (!bar)
        return nullptr;

    QStringList parts = path.split(QLatin1Char('/'), Qt::SkipEmptyParts);
    QMenu* current = nullptr;
    QMenu* parentMenu = nullptr;

    // Menus matched by title; normalize away mnemonic markers and case so
    // "&View" matches "view".
    const auto menuTitleMatches = [](const QString& title, const QString& part)
    {
        QString normalized = title;
        normalized.remove(QLatin1Char('&'));
        return normalized.compare(part, Qt::CaseInsensitive) == 0;
    };

    for (const QString& part : parts)
    {
        // Look for an existing top-level or sub-menu matching part.
        QMenu* found = nullptr;
        if (!current)
        {
            for (QAction* act : bar->actions())
            {
                QMenu* m = act->menu();
                if (m && menuTitleMatches(m->title(), part))
                {
                    found = m;
                    break;
                }
            }
            current = found;
            parentMenu = nullptr;
        }
        else
        {
            for (QAction* act : current->actions())
            {
                QMenu* m = act->menu();
                if (m && menuTitleMatches(m->title(), part))
                {
                    found = m;
                    break;
                }
            }
            current = found;
        }

        if (!current)
        {
            current = new QMenu(part, bar);
            if (!parentMenu)
                bar->addMenu(current);
            else
                parentMenu->addMenu(current);
        }
        parentMenu = current;
    }
    return current;
}

void PluginHost::addMenuAction(const QString& menuPath, QAction* action)
{
    if (!action)
        return;
    QMenu* m = menu(menuPath);
    if (m)
        m->addAction(action);
}

QString PluginHost::addDockWidget(QWidget* widget, Qt::DockWidgetArea area)
{
    if (!m_mainWindow || !widget || !isWidgetFrontend())
        return QString();

    QDockWidget* dock = nullptr;

    // If the widget is already a QDockWidget, add it directly instead of
    // wrapping it in another QDockWidget. This lets plugins contribute
    // QDockWidget-based panels (e.g. TerminalPanel) without nesting.
    if (auto* existing = qobject_cast<QDockWidget*>(widget))
    {
        dock = existing;
    }
    else
    {
        dock = new QDockWidget(m_mainWindow);
        dock->setWidget(widget);
    }

    dock->setObjectName(dock->objectName().isEmpty() ? QStringLiteral("plugin-dock-%1").arg(++m_dockCounter) : dock->objectName());
    m_mainWindow->addDockWidget(area, dock);

    QString dockId = dock->objectName();
    DockEntry entry;
    entry.dock = dock;
    entry.id = dockId;
    m_pluginDocks.insert(dockId, entry);
    m_pluginDockOwners.insert(dockId, m_currentPluginId);

    return dockId;
}

void PluginHost::registerPanel(const QString& id, QWidget* widget)
{
    if (id.isEmpty() || !widget)
        return;
    m_pluginPanels.insert(id, widget);
}

void PluginHost::unregisterPanel(const QString& id)
{
    m_pluginPanels.remove(id);
}

QWidget* PluginHost::panel(const QString& id) const
{
    return m_pluginPanels.value(id, nullptr);
}

void PluginHost::registerObject(const QString& id, QObject* object)
{
    if (id.isEmpty() || !object)
        return;
    m_pluginObjects.insert(id, object);
}

void PluginHost::unregisterObject(const QString& id)
{
    m_pluginObjects.remove(id);
}

QObject* PluginHost::object(const QString& id) const
{
    return m_pluginObjects.value(id, nullptr);
}

bool PluginHost::removeDockWidget(const QString& dockId)
{
    if (!m_mainWindow || dockId.isEmpty())
        return false;

    DockEntry entry = m_pluginDocks.take(dockId);
    if (!entry.dock)
        return false;

    m_mainWindow->removeDockWidget(entry.dock);
    // Unregister any panel that pointed at this dock so host->panel() no
    // longer returns a dangling (deleted) widget.
    for (auto it = m_pluginPanels.begin(); it != m_pluginPanels.end();)
    {
        if (it.value() == entry.dock)
            it = m_pluginPanels.erase(it);
        else
            ++it;
    }
    entry.dock->deleteLater();
    return true;
}

QVariant PluginHost::pluginSetting(const QString& pluginId, const QString& key, const QVariant& defaultValue) const
{
    return m_settingsManager ? m_settingsManager->pluginSetting(pluginId, key, defaultValue) : defaultValue;
}

void PluginHost::setPluginSetting(const QString& pluginId, const QString& key, const QVariant& value)
{
    if (m_settingsManager)
        m_settingsManager->setPluginSetting(pluginId, key, value);
}

void PluginHost::removePluginSetting(const QString& pluginId, const QString& key)
{
    if (m_settingsManager)
        m_settingsManager->removePluginSetting(pluginId, key);
}

QStringList PluginHost::pluginKeys(const QString& pluginId) const
{
    return m_settingsManager ? m_settingsManager->pluginKeys(pluginId) : QStringList();
}

void PluginHost::registerLspServer(const QString& language, const QString& command, const QStringList& args)
{
    if (m_lspServerManager)
        m_lspServerManager->registerPluginServer(language, command, args);
}

bool PluginHost::registerLanguage(const QString& languageId, const QString& lexerClassName, LexerFactory factory)
{
    if (languageId.isEmpty() || !factory)
        return false;
    if (!registerPluginLanguage(languageId, lexerClassName, factory))
        return false;
    m_pluginRegistrations[m_currentPluginId].languageIds.append(languageId);
    return true;
}

bool PluginHost::registerLanguageKeywords(const QString& languageId, const QStringList& keywords)
{
    if (languageId.isEmpty() || keywords.isEmpty())
        return false;
    if (!registerPluginLanguageKeywords(languageId, keywords))
        return false;
    m_pluginRegistrations[m_currentPluginId].languageIds.append(languageId);
    return true;
}

bool PluginHost::registerEncoding(const QString& displayName, const QString& encodingName)
{
    if (displayName.isEmpty() || encodingName.isEmpty())
        return false;
    if (!registerPluginEncoding(displayName, encodingName))
        return false;
    m_pluginRegistrations[m_currentPluginId].encodingNames.append(displayName);
    return true;
}

bool PluginHost::registerExtension(const QString& extension, const QString& languageId)
{
    if (extension.isEmpty() || languageId.isEmpty())
        return false;
    if (!SettingsManager::addExtensionMapping(extension, languageId))
        return false;
    m_pluginRegistrations[m_currentPluginId].extensions.append(extension);
    return true;
}

bool PluginHost::registerFileNameMapping(const QString& fileName, const QString& languageId)
{
    if (fileName.isEmpty() || languageId.isEmpty())
        return false;
    if (!SettingsManager::addFileNameMapping(fileName, languageId))
        return false;
    m_pluginRegistrations[m_currentPluginId].fileNames.append(fileName);
    return true;
}

bool PluginHost::registerSnippet(const QString& name, const QString& prefix, const QStringList& body, const QString& description,
                                 const QString& scope)
{
    if (!SnippetManager::instance().addSnippet(name, prefix, body, description, scope))
        return false;
    m_pluginRegistrations[m_currentPluginId].snippetNames.append(name);
    return true;
}

bool PluginHost::unregisterSnippet(const QString& name)
{
    if (!SnippetManager::instance().removeSnippet(name))
        return false;
    m_pluginRegistrations[m_currentPluginId].snippetNames.removeAll(name);
    return true;
}

bool PluginHost::registerTheme(const QString& themeId, const ThemeColors& colors)
{
    qDebug("PluginHost::registerTheme: id='%s', calling registerPluginTheme", qPrintable(themeId));
    bool ok = registerPluginTheme(themeId, colors);
    qDebug("PluginHost::registerTheme: registerPluginTheme returned %d", ok);
    if (!ok)
        return false;
    m_pluginRegistrations[m_currentPluginId].themeIds.append(themeId);
    return true;
}

bool PluginHost::unregisterTheme(const QString& themeId)
{
    if (!unregisterPluginTheme(themeId))
        return false;
    m_pluginRegistrations[m_currentPluginId].themeIds.removeAll(themeId);
    return true;
}

bool PluginHost::isWidgetFrontend() const
{
    return m_mainWindow != nullptr;
}
