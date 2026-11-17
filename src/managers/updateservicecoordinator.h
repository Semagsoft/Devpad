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
#ifndef UPDATESERVICECOORDINATOR_H
#define UPDATESERVICECOORDINATOR_H

#include <QObject>

class QStatusBar;
class QWidget;
class UpdateChecker;

/**
 * @brief Wires UpdateChecker signals to status-bar messages and dialogs.
 *
 * Extracted from MainWindow: the update-check UI flow (clear the status bar,
 * show the "update available" / "up to date" / "check failed" dialogs) is a
 * self-contained responsibility. When the user asks to retry a failed check,
 * @ref retryCheckRequested is emitted and the caller decides what to do.
 */
class UpdateServiceCoordinator : public QObject
{
    Q_OBJECT

public:
    explicit UpdateServiceCoordinator(UpdateChecker* checker, QStatusBar* statusBar, QWidget* dialogParent, QObject* parent = nullptr);

signals:
    /** @brief The user chose "Retry" on the update-check-failed dialog. */
    void retryCheckRequested();

private:
    UpdateChecker* m_checker;
    QStatusBar* m_statusBar;
    QWidget* m_dialogParent;
};

#endif // UPDATESERVICECOORDINATOR_H
