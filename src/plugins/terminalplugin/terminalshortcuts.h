/*
 * Devpad - Terminal keyboard shortcut resolution helpers
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
#ifndef TERMINALSHORTCUTS_H
#define TERMINALSHORTCUTS_H

/**
 * Shared shortcut-resolution helpers for the terminal backends.
 *
 * The terminal claims Qt::ShortcutOverride for keys the shell/TUI needs, so
 * application-wide window shortcuts do not fire while the user is typing in
 * the terminal. A small allowlist stays global: the terminal toggle and the
 * terminal copy/paste actions, both remappable in Options -> Keyboard.
 */

#include "settingsmanager.h"

#include <QKeyCombination>
#include <QKeyEvent>
#include <QKeySequence>
#include <QString>

namespace TerminalShortcuts
{
/**
 * Effective shortcut for an action: the user-configured one when set,
 * otherwise the built-in default passed as @p fallback.
 */
inline QKeySequence effectiveShortcut(const QString& actionId, const QKeySequence& fallback)
{
    const QString stored = SettingsManager::instance().shortcutForAction(actionId);
    if (!stored.isEmpty())
        return QKeySequence(stored);
    return fallback;
}

/** True when @p event is exactly the single-combo @p shortcut. */
inline bool matchesShortcut(const QKeyEvent& event, const QKeySequence& shortcut)
{
    if (shortcut.isEmpty() || shortcut.count() != 1)
        return false;
    const QKeyCombination combo = shortcut[0];
    return event.key() == combo.key() && event.modifiers() == combo.keyboardModifiers();
}

/**
 * True when @p event must remain an application-global shortcut while the
 * terminal has focus: the terminal panel toggle and the terminal copy/paste
 * actions. Everything else is claimed by the terminal via ShortcutOverride.
 */
inline bool allowsGlobalShortcut(const QKeyEvent& event)
{
    if (matchesShortcut(event, effectiveShortcut(QStringLiteral("View.TerminalPanel"), QKeySequence(QStringLiteral("Ctrl+`")))))
        return true;
    if (matchesShortcut(event, effectiveShortcut(QStringLiteral("Terminal.Copy"), QKeySequence(QStringLiteral("Ctrl+Shift+C")))))
        return true;
    if (matchesShortcut(event, effectiveShortcut(QStringLiteral("Terminal.Paste"), QKeySequence(QStringLiteral("Ctrl+Shift+V")))))
        return true;
    return false;
}
} // namespace TerminalShortcuts

#endif // TERMINALSHORTCUTS_H
