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
#ifndef TERMINALPANELCOORDINATOR_H
#define TERMINALPANELCOORDINATOR_H

#include <QDockWidget>
#include <QObject>
#include <QPointer>

class ActionManager;
class PluginManager;
class QMainWindow;
class QTabWidget;

/**
 * @brief Locates and wires the plugin-owned Terminal panel.
 *
 * Extracted from MainWindow: after the Terminal plugin loads (or is
 * re-enabled), the panel it registers with the plugin host is discovered
 * here. The coordinator owns the dock's lifecycle handling (visibility ->
 * action state + persisted setting), applies the persisted position and
 * visibility on attach, and exposes the dock for the rest of the UI.
 */
class TerminalPanelCoordinator : public QObject
{
    Q_OBJECT

public:
    explicit TerminalPanelCoordinator(ActionManager* actionManager, PluginManager* pluginManager, QTabWidget* tabWidget, QMainWindow* uiParent,
                                      QObject* parent = nullptr);

    /** @brief Find the plugin's terminal panel and wire it up. Safe to call again after plugin re-enable. */
    void attach();

    /** @brief Forget the panel (plugin disabled). The plugin owns and deletes the panel itself. */
    void detach();

    /** @brief Apply the persisted visibility/position from SettingsManager to the dock. */
    void applySettings();

    /** @brief Re-apply the persisted dock position (used after settings that move the panel). */
    void applyPosition();

    /** @brief The plugin-owned dock, or nullptr if the plugin is not loaded/enabled. */
    QDockWidget* dock() const
    {
        return m_dock;
    }

private slots:
    void handleTerminalStarted();

private:

    ActionManager* m_actionManager;
    PluginManager* m_pluginManager;
    QTabWidget* m_tabWidget;
    QMainWindow* m_uiParent;
    QPointer<QDockWidget> m_dock;
};

#endif // TERMINALPANELCOORDINATOR_H
