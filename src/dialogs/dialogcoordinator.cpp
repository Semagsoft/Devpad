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
#include "dialogcoordinator.h"

#include "findinfilesdialog.h"
#include "findsymbolsdialog.h"
#include "projectpanel.h"
#include "tabmanager.h"

#include <QDir>
#include <QWidget>

DialogCoordinator::DialogCoordinator(TabManager* tabManager, ProjectPanel* projectPanel, QObject* parent)
    : QObject(parent), m_tabManager(tabManager), m_projectPanel(projectPanel)
{
}

void DialogCoordinator::findInFiles()
{
    if (!m_findInFilesDialog)
    {
        QString defaultDir = m_projectPanel->isVisible() ? m_projectPanel->rootPath() : QDir::homePath();

        m_findInFilesDialog = new FindInFilesDialog(defaultDir, parentWidget());
        connect(m_findInFilesDialog, &FindInFilesDialog::navigateToResult, this,
                [this](const QString& filePath, int lineNumber) { emit navigateToResult(filePath, lineNumber - 1, 0); });
        connect(m_findInFilesDialog, &QDialog::finished, this,
                [this]()
                {
                    m_findInFilesDialog->deleteLater();
                    m_findInFilesDialog = nullptr;
                });
    }

    if (auto* editor = m_tabManager->currentEditor())
    {
        QString selected = editor->selectedText();
        if (!selected.isEmpty())
            m_findInFilesDialog->startSearch(selected, m_projectPanel->isVisible() ? m_projectPanel->rootPath() : QDir::homePath());
    }

    m_findInFilesDialog->show();
    m_findInFilesDialog->raise();
    m_findInFilesDialog->activateWindow();
}

void DialogCoordinator::findSymbols(lsp::LspServerManager* lspManager)
{
    if (!m_findSymbolsDialog)
    {
        m_findSymbolsDialog = new FindSymbolsDialog(lspManager, parentWidget());
        connect(m_findSymbolsDialog, &FindSymbolsDialog::navigateToSymbol, this, &DialogCoordinator::navigateToResult);
        connect(m_findSymbolsDialog, &QDialog::finished, this,
                [this]()
                {
                    m_findSymbolsDialog->deleteLater();
                    m_findSymbolsDialog = nullptr;
                });
    }
    m_findSymbolsDialog->show();
    m_findSymbolsDialog->raise();
    m_findSymbolsDialog->activateWindow();
    m_findSymbolsDialog->setFocus();
}

QWidget* DialogCoordinator::parentWidget() const
{
    return qobject_cast<QWidget*>(parent());
}
