#include "editorcontainer.h"

#include "codeeditor.h"
#include "inlinefindbar.h"

#include <QVBoxLayout>

EditorContainer::EditorContainer(CodeEditor* editor, QWidget* parent) : QWidget(parent), m_editor(editor)
{
    m_layout = new QVBoxLayout(this);
    m_layout->setContentsMargins(0, 0, 0, 0);
    m_layout->setSpacing(0);

    m_findBar = new InlineFindBar(m_editor, this);
    m_findBar->setVisible(false);

    m_layout->addWidget(m_findBar);
    m_layout->addWidget(m_editor, 1);

    connect(m_findBar, &InlineFindBar::closed, this, [this]() { m_findBar->hide(); });
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
