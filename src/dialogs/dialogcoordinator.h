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
#ifndef DIALOGCOORDINATOR_H
#define DIALOGCOORDINATOR_H

#include <QObject>
#include <QPointer>

class FindInFilesDialog;
class FindSymbolsDialog;
class ProjectPanel;
class TabManager;

namespace lsp
{
class LspServerManager;
} // namespace lsp

/**
 * @brief Owns the long-lived find dialogs (find-in-files, find-symbols).
 *
 * The coordinator keeps dialog construction, lifetime (delete-on-close) and
 * show/raise/activate handling in one place. Navigation requests are emitted
 * as @ref navigateToResult signals; MainWindow handles file loading and
 * tab switching itself.
 */
class DialogCoordinator : public QObject
{
    Q_OBJECT

public:
    explicit DialogCoordinator(TabManager* tabManager, ProjectPanel* projectPanel, QObject* parent = nullptr);

    void findInFiles();
    void findSymbols(lsp::LspServerManager* lspManager);

signals:
    /**
     * @brief Navigate to a position in a file (empty @p filePath means just cursor).
     *        @p line and @p column are 0-based editor coordinates.
     */
    void navigateToResult(const QString& filePath, int line, int column);

private:
    QWidget* parentWidget() const;

    TabManager* m_tabManager;
    ProjectPanel* m_projectPanel;
    QPointer<FindInFilesDialog> m_findInFilesDialog;
    QPointer<FindSymbolsDialog> m_findSymbolsDialog;
};

#endif // DIALOGCOORDINATOR_H
