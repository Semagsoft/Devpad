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
#include "lsp/lspservermanager.h"
#include "mainwindow.h"
#include "managers/settingsmanager.h"
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

        forwardEditorSignals();
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
            [callback, act]()
            {
                if (callback)
                    callback();
            });
    m_pluginActions.insert(id, act);

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

    for (const QString& part : parts)
    {
        // Look for an existing top-level or sub-menu matching part.
        QMenu* found = nullptr;
        if (!current)
        {
            for (QAction* act : bar->actions())
            {
                if (QMenu* m = act->menu())
                {
                    if (m->title() == part)
                    {
                        found = m;
                        break;
                    }
                }
            }
            current = found;
            parentMenu = nullptr;
        }
        else
        {
            for (QAction* act : current->actions())
            {
                if (QMenu* m = act->menu())
                {
                    if (m->title() == part)
                    {
                        found = m;
                        break;
                    }
                }
            }
            current = found;
        }

        if (!current)
        {
            current = new QMenu(part, const_cast<QMenuBar*>(bar));
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

    auto* dock = new QDockWidget(m_mainWindow);
    dock->setWidget(widget);
    dock->setObjectName(QStringLiteral("plugin-dock-%1").arg(++m_dockCounter));
    m_mainWindow->addDockWidget(area, dock);

    QString dockId = dock->objectName();
    DockEntry entry;
    entry.dock = dock;
    entry.id = dockId;
    m_pluginDocks.insert(dockId, entry);

    // Keep the action checked state in sync with visibility.
    connect(dock, &QDockWidget::visibilityChanged, dock, [dock](bool visible) { dock->setVisible(visible); });

    return dockId;
}

bool PluginHost::removeDockWidget(const QString& dockId)
{
    if (!m_mainWindow || dockId.isEmpty())
        return false;

    DockEntry entry = m_pluginDocks.take(dockId);
    if (!entry.dock)
        return false;

    m_mainWindow->removeDockWidget(entry.dock);
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

bool PluginHost::isWidgetFrontend() const
{
    return m_mainWindow != nullptr;
}
