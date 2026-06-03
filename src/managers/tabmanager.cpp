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
#include "tabmanager.h"

#include "settingsmanager.h"

#include <QFileInfo>
#include <QFont>
#include <QIcon>
#include <QPointer>
#include <QTabBar>
#include <QToolButton>

#include <Qsci/qsciscintilla.h>

TabManager::TabManager(QTabWidget* primaryWidget, QObject* parent) : QObject(parent), m_primaryWidget(primaryWidget), m_activePane(primaryWidget)
{
    m_panes.append(primaryWidget);
    connect(primaryWidget, &QTabWidget::currentChanged, this,
            [this](int localIdx)
            {
                int globalIdx = localIdx;
                for (QTabWidget* p : m_panes)
                {
                    if (p == m_primaryWidget)
                        break;
                    globalIdx += p->count();
                }
                emit currentChanged(globalIdx);
            });
    connect(primaryWidget, &QTabWidget::tabCloseRequested, this,
            [this](int localIdx)
            {
                int globalIdx = localIdx;
                for (QTabWidget* p : m_panes)
                {
                    if (p == m_primaryWidget)
                        break;
                    globalIdx += p->count();
                }
                emit tabCloseRequested(globalIdx);
            });
    updateTabBarVisibility();
}

CodeEditor* TabManager::currentEditor() const
{
    return qobject_cast<CodeEditor*>(m_activePane->currentWidget());
}

CodeEditor* TabManager::editorAt(int index) const
{
    int globalIdx = 0;
    for (QTabWidget* pane : m_panes)
    {
        if (index < globalIdx + pane->count())
        {
            return qobject_cast<CodeEditor*>(pane->widget(index - globalIdx));
        }
        globalIdx += pane->count();
    }
    return nullptr;
}

CodeEditor* TabManager::findEditorByFileName(const QString& fileName) const
{
    for (QTabWidget* pane : m_panes)
    {
        for (int i = 0; i < pane->count(); ++i)
        {
            CodeEditor* editor = qobject_cast<CodeEditor*>(pane->widget(i));
            if (editor && editor->fileName() == fileName)
            {
                return editor;
            }
        }
    }
    return nullptr;
}

int TabManager::count() const
{
    int total = 0;
    for (QTabWidget* pane : m_panes)
    {
        total += pane->count();
    }
    return total;
}

int TabManager::currentIndex() const
{
    return m_activePane->currentIndex();
}

int TabManager::indexOf(CodeEditor* editor) const
{
    int globalIdx = 0;
    for (QTabWidget* pane : m_panes)
    {
        int localIdx = pane->indexOf(editor);
        if (localIdx >= 0)
            return globalIdx + localIdx;
        globalIdx += pane->count();
    }
    return -1;
}

CodeEditor* TabManager::createEditor()
{
    CodeEditor* editor = new CodeEditor(m_activePane);
    return editor;
}

void TabManager::addEditor(CodeEditor* editor, const QString& title)
{
    m_activePane->addTab(editor, title);
    int idx = m_activePane->indexOf(editor);
    updateCloseButton(idx, m_activePane, SettingsManager::instance().closeButtonMode());
    m_activePane->setCurrentWidget(editor);
    editor->setFocus();
    updateTabBarVisibility();
    emit editorCreated(editor);
}

void TabManager::removeEditor(int index)
{
    int globalIdx = 0;
    for (QTabWidget* pane : m_panes)
    {
        if (index < globalIdx + pane->count())
        {
            int localIdx = index - globalIdx;
            auto* editor = qobject_cast<CodeEditor*>(pane->widget(localIdx));
            if (!editor)
                return;
            removeCloseButtons(localIdx, pane);
            pane->removeTab(localIdx);
            for (int i = localIdx; i < pane->count(); ++i)
            {
                updateCloseButton(i, pane, SettingsManager::instance().closeButtonMode());
            }
            emit editorClosed(editor);
            editor->deleteLater();
            updateTabBarVisibility();
            return;
        }
        globalIdx += pane->count();
    }
}

bool TabManager::closeEditor(int index)
{
    int globalIdx = 0;
    for (QTabWidget* pane : m_panes)
    {
        if (index < globalIdx + pane->count())
        {
            int localIdx = index - globalIdx;
            auto* editor = qobject_cast<CodeEditor*>(pane->widget(localIdx));
            if (!editor)
                return false;
            removeCloseButtons(localIdx, pane);
            pane->removeTab(localIdx);
            for (int i = localIdx; i < pane->count(); ++i)
            {
                updateCloseButton(i, pane, SettingsManager::instance().closeButtonMode());
            }
            emit editorClosed(editor);
            editor->deleteLater();
            updateTabBarVisibility();
            return true;
        }
        globalIdx += pane->count();
    }
    return false;
}

void TabManager::addPane(QTabWidget* pane)
{
    if (m_panes.contains(pane))
        return;
    m_panes.append(pane);
    connect(pane, &QTabWidget::currentChanged, this,
            [this, pane](int localIdx)
            {
                int globalIdx = localIdx;
                for (QTabWidget* p : m_panes)
                {
                    if (p == pane)
                        break;
                    globalIdx += p->count();
                }
                emit currentChanged(globalIdx);
            });
    connect(pane, &QTabWidget::tabCloseRequested, this,
            [this, pane](int localIdx)
            {
                int globalIdx = localIdx;
                for (QTabWidget* p : m_panes)
                {
                    if (p == pane)
                        break;
                    globalIdx += p->count();
                }
                emit tabCloseRequested(globalIdx);
            });
    updateAllCloseButtons(SettingsManager::instance().closeButtonMode());
}

void TabManager::removePane(QTabWidget* pane)
{
    if (pane == m_primaryWidget)
        return;
    m_panes.removeOne(pane);
    if (m_activePane == pane)
    {
        m_activePane = m_primaryWidget;
    }
}

void TabManager::setActivePane(QTabWidget* pane)
{
    if (m_panes.contains(pane))
    {
        m_activePane = pane;
    }
}

QTabWidget* TabManager::paneForEditor(CodeEditor* editor) const
{
    for (QTabWidget* pane : m_panes)
    {
        if (pane->indexOf(editor) >= 0)
            return pane;
    }
    return nullptr;
}

void TabManager::updateTabTitle(CodeEditor* editor)
{
    QTabWidget* pane = paneForEditor(editor);
    if (!pane)
        return;
    int index = pane->indexOf(editor);
    if (index >= 0)
    {
        QString baseName = getFileBaseName(editor->fileName());
        if (editor->isModified())
        {
            baseName = "*" + baseName;
        }
        if (editor->isReadOnlyMode())
        {
            baseName += tr(" [Read Only]");
        }
        pane->setTabText(index, baseName);
    }
}

QString TabManager::getFileBaseName(const QString& fullPath) const
{
    return QFileInfo(fullPath).fileName();
}

void TabManager::updateCloseButton(int tabIndex, QTabWidget* pane, CloseButtonMode closeButtonMode)
{
    QTabBar* tabBar = pane->tabBar();
    if (!tabBar)
        return;
    QToolButton* closeButton = qobject_cast<QToolButton*>(tabBar->tabButton(tabIndex, QTabBar::RightSide));
    if (!closeButton)
    {
        closeButton = qobject_cast<QToolButton*>(tabBar->tabButton(tabIndex, QTabBar::LeftSide));
    }
    if (!closeButton)
    {
        closeButton = new QToolButton(tabBar);
        closeButton->setFixedSize(24, 24);
        QIcon closeIcon(":/icons/File/close.svg");
        if (closeIcon.isNull())
        {
            closeButton->setText("x");
            closeButton->setFont(QFont("Arial", 12));
        }
        else
        {
            closeButton->setIcon(closeIcon);
        }
        closeButton->setCursor(Qt::PointingHandCursor);
        closeButton->setToolTip(tr("Close"));
        closeButton->setStyleSheet("QToolButton { border: none; background: transparent; }");
        QPointer<QToolButton> buttonGuard(closeButton);
        connect(closeButton, &QToolButton::clicked, this,
                [this, buttonGuard, pane]()
                {
                    if (!buttonGuard)
                        return;
                    int tabIdx = buttonGuard->property("tabIndex").toInt();
                    if (tabIdx < 0 || tabIdx >= pane->count())
                        return;
                    int globalIdx = tabIdx;
                    for (QTabWidget* p : m_panes)
                    {
                        if (p == pane)
                            break;
                        globalIdx += p->count();
                    }
                    emit tabCloseRequested(globalIdx);
                });
    }

    closeButton->setProperty("tabIndex", tabIndex);

    tabBar->setTabButton(tabIndex, QTabBar::RightSide, nullptr);
    tabBar->setTabButton(tabIndex, QTabBar::LeftSide, nullptr);

    if (closeButtonMode == CloseButtonMode::Right)
    {
        tabBar->setTabButton(tabIndex, QTabBar::RightSide, closeButton);
    }
    else
    {
        tabBar->setTabButton(tabIndex, QTabBar::LeftSide, closeButton);
    }
}

void TabManager::updateAllCloseButtons(CloseButtonMode closeButtonMode)
{
    for (QTabWidget* pane : m_panes)
    {
        for (int i = 0; i < pane->count(); ++i)
        {
            updateCloseButton(i, pane, closeButtonMode);
        }
    }
}

void TabManager::applySettingsToAll(const SettingsManager::EditorSettings& settings)
{
    QFont font(settings.defaultFontFamily, settings.defaultFontSize);
    for (QTabWidget* pane : m_panes)
    {
        for (int i = 0; i < pane->count(); ++i)
        {
            CodeEditor* editor = qobject_cast<CodeEditor*>(pane->widget(i));
            if (editor)
            {
                editor->applyFont(font);
                editor->setLineNumbersVisible(settings.showLineNumbers);
                editor->setScrollPastEnd(settings.scrollPastContent);
                editor->setCodeFolding(settings.codeCollapsing);
                editor->setWrapMode(settings.wordWrap ? QsciScintilla::WrapWord : QsciScintilla::WrapNone);
                editor->applyTheme(settings.theme);
                editor->setupAutoCompletion(settings.autoCompletionEnabled, settings.autoCompletionThreshold, settings.autoCompletionCaseSensitive);
                editor->setAutoCloseBrackets(settings.autoCloseBrackets);
                editor->setTabWidth(settings.tabWidth);
                editor->setCursorStyle(settings.cursorStyle);
                editor->setCursorBlinking(settings.cursorBlinking);
                editor->setHighlightCurrentLine(settings.highlightCurrentLine);
                editor->setVerticalEdge(settings.verticalEdgeEnabled, settings.verticalEdgeColumn);
                updateTabTitle(editor);
            }
        }
    }
}

void TabManager::removeCloseButtons(int tabIndex, QTabWidget* pane)
{
    QTabBar* tabBar = pane->tabBar();
    if (!tabBar)
        return;

    QToolButton* rightButton = qobject_cast<QToolButton*>(tabBar->tabButton(tabIndex, QTabBar::RightSide));
    QToolButton* leftButton = qobject_cast<QToolButton*>(tabBar->tabButton(tabIndex, QTabBar::LeftSide));

    tabBar->setTabButton(tabIndex, QTabBar::RightSide, nullptr);
    tabBar->setTabButton(tabIndex, QTabBar::LeftSide, nullptr);

    if (rightButton)
    {
        disconnect(rightButton, &QToolButton::clicked, this, nullptr);
        rightButton->deleteLater();
    }
    if (leftButton && leftButton != rightButton)
    {
        disconnect(leftButton, &QToolButton::clicked, this, nullptr);
        leftButton->deleteLater();
    }
}

void TabManager::updateTabBarVisibility()
{
    for (QTabWidget* pane : m_panes)
    {
        QTabBar* tabBar = pane->tabBar();
        if (!tabBar)
            continue;

        TabDisplayMode mode = SettingsManager::instance().tabDisplayMode();
        switch (mode)
        {
        case TabDisplayMode::AlwaysShow:
            tabBar->setVisible(true);
            break;
        case TabDisplayMode::ShowTwoPlus:
            tabBar->setVisible(pane->count() > 1);
            break;
        case TabDisplayMode::NeverShow:
            tabBar->setVisible(false);
            break;
        }
    }
}
