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
#include "managers/terminalpanelcoordinator.h"

#include "managers/actionmanager.h"
#include "managers/settingsmanager.h"
#include "plugins/pluginhost.h"
#include "plugins/pluginmanager.h"

#include <QMainWindow>
#include <QMetaMethod>
#include <QTabWidget>
#include <QToolButton>
#include <QWidget>

TerminalPanelCoordinator::TerminalPanelCoordinator(ActionManager* actionManager, PluginManager* pluginManager, QTabWidget* tabWidget,
                                                   QMainWindow* uiParent, QObject* parent)
    : QObject(parent), m_actionManager(actionManager), m_pluginManager(pluginManager), m_tabWidget(tabWidget), m_uiParent(uiParent)
{
}

void TerminalPanelCoordinator::attach()
{
    if (!m_pluginManager)
        return;

    auto* panel = m_pluginManager->host()->panel(QStringLiteral("terminal"));
    if (panel && panel->inherits("TerminalPanel"))
    {
        m_dock = qobject_cast<QDockWidget*>(panel);
        // The Terminal panel exists — show the View menu action and status bar button.
        m_actionManager->terminalPanelAct()->setVisible(true);
        m_actionManager->terminalPanelButton()->setVisible(true);
    }
    else
    {
        m_dock = nullptr;
        m_actionManager->terminalPanelAct()->setVisible(false);
        m_actionManager->terminalPanelButton()->setVisible(false);
        return;
    }

    // Connect terminal lifecycle signals (now that the panel exists).
    auto* meta = m_dock->metaObject();
    int startedIdx = meta->indexOfSignal("terminalStarted()");
    if (startedIdx >= 0)
    {
        QMetaMethod signalMethod = meta->method(startedIdx);
        auto* myMeta = metaObject();
        int slotIdx = myMeta->indexOfSlot("handleTerminalStarted()");
        if (slotIdx >= 0)
            QObject::connect(m_dock, signalMethod, this, myMeta->method(slotIdx));
    }

    connect(m_dock, &QDockWidget::visibilityChanged, this,
            [this](bool visible)
            {
                if (SettingsManager::instance().terminalPanelPosition() != TerminalPanelPosition::Tab)
                {
                    m_actionManager->terminalPanelAct()->setChecked(visible);
                    m_actionManager->terminalPanelButton()->setChecked(visible);
                    SettingsManager::instance().setShowTerminalPanel(visible);
                }
            });

    // Apply terminal settings now that the panel is available.
    applySettings();
}

void TerminalPanelCoordinator::detach()
{
    // The plugin owns and deletes the panel on unload; just drop our reference.
    m_dock = nullptr;
    m_actionManager->terminalPanelAct()->setVisible(false);
    m_actionManager->terminalPanelButton()->setVisible(false);
}

void TerminalPanelCoordinator::applyPosition()
{
    if (!m_dock)
        return;
    bool visible = SettingsManager::instance().showTerminalPanel();
    m_actionManager->terminalPanelAct()->setChecked(visible);
    m_actionManager->terminalPanelButton()->setChecked(visible);

    int pos = static_cast<int>(SettingsManager::instance().terminalPanelPosition());
    QMetaObject::invokeMethod(m_dock, "applyPosition", Q_ARG(int, pos), Q_ARG(QTabWidget*, m_tabWidget), Q_ARG(QMainWindow*, m_uiParent));
}

void TerminalPanelCoordinator::applySettings()
{
    if (!m_dock)
        return;

    if (SettingsManager::instance().showTerminalPanel())
    {
        QMetaObject::invokeMethod(m_dock, "toggle", Q_ARG(QTabWidget*, m_tabWidget), Q_ARG(QMainWindow*, m_uiParent));
        m_actionManager->terminalPanelAct()->setChecked(true);
        m_actionManager->terminalPanelButton()->setChecked(true);
    }
    else
    {
        int pos = static_cast<int>(SettingsManager::instance().terminalPanelPosition());
        QMetaObject::invokeMethod(m_dock, "applyPosition", Q_ARG(int, pos), Q_ARG(QTabWidget*, m_tabWidget), Q_ARG(QMainWindow*, m_uiParent));
    }
}

void TerminalPanelCoordinator::handleTerminalStarted()
{
    if (!m_dock)
        return;
    auto disableDrops = [](auto&& self, QObject* obj) -> void
    {
        if (auto* w = qobject_cast<QWidget*>(obj))
            w->setAcceptDrops(false);
        for (QObject* child : obj->children())
            self(self, child);
    };
    disableDrops(disableDrops, m_dock);
}
