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
#ifndef TABMANAGER_H
#define TABMANAGER_H

#include "codeeditor.h"
#include "settingsmanager.h"
#include "widgets/editorcontainer.h"

#include <QHash>
#include <QList>
#include <QObject>
#include <QPointer>
#include <QSet>
#include <QString>
#include <QTabWidget>
#include <QToolButton>

// qHash overload for QPointer to enable QSet<QPointer<CodeEditor>>
inline size_t qHash(const QPointer<CodeEditor>& ptr, size_t seed = 0) noexcept
{
    return qHash(ptr.data(), seed);
}

class TabManager : public QObject
{
    Q_OBJECT

public:
    explicit TabManager(QTabWidget* primaryWidget, QObject* parent = nullptr);

    CodeEditor* currentEditor() const;
    CodeEditor* editorAt(int index) const;
    CodeEditor* findEditorByFileName(const QString& fileName) const;
    int indexOf(CodeEditor* editor) const;
    int count() const;
    int currentIndex() const;

    CodeEditor* createEditor();
    void addEditor(CodeEditor* editor, const QString& title);
    void removeEditor(int index);
    bool closeEditor(int index);

    void addPane(QTabWidget* pane);
    void removePane(QTabWidget* pane);
    void setActivePane(QTabWidget* pane);
    QTabWidget* activePane() const
    {
        return m_activePane;
    }
    QList<QTabWidget*> panes() const
    {
        return m_panes;
    }

    void updateTabTitle(CodeEditor* editor);
    void updateCloseButton(int tabIndex, QTabWidget* pane, CloseButtonMode closeButtonMode);
    void updateAllCloseButtons(CloseButtonMode closeButtonMode);
    void updateTabBarVisibility();

    void applySettingsToAll(const SettingsManager::EditorSettings& settings);

    void setTabPinned(CodeEditor* editor, bool pinned);
    bool isTabPinned(CodeEditor* editor) const;
    QStringList pinnedFiles() const;
    void setPinnedFiles(const QStringList& files);

    EditorContainer* containerFor(CodeEditor* editor) const;

signals:
    void editorCreated(CodeEditor* editor);
    void editorClosed(CodeEditor* editor);
    void currentChanged(int index);
    void tabCloseRequested(int index);

private:
    QList<QTabWidget*> m_panes;
    QTabWidget* m_primaryWidget;
    QTabWidget* m_activePane;
    // QPointer auto-nulls if an editor is deleted outside normal close path
    QSet<QPointer<CodeEditor>> m_pinnedEditors;
    int localToGlobalIndex(int localIdx, QTabWidget* pane) const;
    QString getFileBaseName(const QString& fullPath) const;
    void removeCloseButtons(int tabIndex, QTabWidget* pane);
    QTabWidget* paneForEditor(CodeEditor* editor) const;
    QHash<CodeEditor*, QPointer<EditorContainer>> m_containers;
};

#endif
