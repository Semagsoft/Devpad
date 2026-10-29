#include "editorcontainer.h"

#include "codeeditor.h"
#include "inlinefindbar.h"
#include "minimap.h"
#include "managers/settingsmanager.h"

#include <QVBoxLayout>
#include <QHBoxLayout>

EditorContainer::EditorContainer(CodeEditor* editor, QWidget* parent) : QWidget(parent), m_editor(editor)
{
    m_layout = new QHBoxLayout(this);
    m_layout->setContentsMargins(0, 0, 0, 0);
    m_layout->setSpacing(0);

    // Center column: find bar stacked above the editor.
    m_centerLayout = new QVBoxLayout();
    m_centerLayout->setContentsMargins(0, 0, 0, 0);
    m_centerLayout->setSpacing(0);

    m_findBar = new InlineFindBar(m_editor, this);
    m_findBar->setVisible(false);

    m_centerLayout->addWidget(m_findBar);
    m_centerLayout->addWidget(m_editor, 1);

    m_layout->addLayout(m_centerLayout, 1);

    m_minimap = new Minimap(m_editor, this);
    m_minimap->setVisible(false);
    m_layout->addWidget(m_minimap, 0);

    connect(m_findBar, &InlineFindBar::closed, this, [this]() { m_findBar->hide(); });

    // Keep the minimap visibility in sync with the editor flag and with
    // global setting changes (e.g. toggling in the Options dialog).
    setMinimapVisible(SettingsManager::instance().showMinimap());
    connect(m_editor, &CodeEditor::minimapVisibleChanged, this,
            [this](bool visible) { setMinimapVisible(visible); });
    connect(&SettingsManager::instance(), &SettingsManager::settingsChanged, this,
            &EditorContainer::applyMinimapSetting);
}

void EditorContainer::applyMinimapSetting()
{
    setMinimapVisible(SettingsManager::instance().showMinimap());
}

void EditorContainer::setMinimapVisible(bool visible)
{
    if (m_minimap->isVisible() == visible)
        return;
    m_minimap->setVisible(visible);
    // The editor column stretches either way; hiding the strip gives the
    // editor the full width.
}

bool EditorContainer::isMinimapVisible() const
{
    return m_minimap->isVisible();
}

void EditorContainer::showFindBar()
{
    QString selected = m_editor->selectedText();
    m_findBar->showFindMode(selected);
}

void EditorContainer::showReplaceBar()
{
    QString selected = m_editor->selectedText();
    m_findBar->showReplaceMode(selected);
}

void EditorContainer::hideFindBar()
{
    m_findBar->closeBar();
}

bool EditorContainer::isFindBarVisible() const
{
    return m_findBar->isVisible();
}
