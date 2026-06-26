#include "searchmanager.h"
#include "codeeditor.h"
#include "tabmanager.h"
#include "widgets/editorcontainer.h"
#include "widgets/inlinefindbar.h"
#include <Qsci/qsciscintilla.h>
#include <QStatusBar>

SearchManager::SearchManager(QMainWindow *parent, TabManager *tabManager)
    : QObject(parent), m_parent(parent), m_tabManager(tabManager) {
}

SearchManager::~SearchManager() {
}

CodeEditor* SearchManager::currentEditor() const {
    return m_tabManager ? m_tabManager->currentEditor() : nullptr;
}

void SearchManager::showFindDialog() {
    auto *editor = currentEditor();
    if (!editor) return;

    auto *container = m_tabManager->containerFor(editor);
    if (container)
        container->showFindBar();
}

void SearchManager::showReplaceDialog() {
    auto *editor = currentEditor();
    if (!editor) return;

    auto *container = m_tabManager->containerFor(editor);
    if (container)
        container->showReplaceBar();
}

void SearchManager::findNext() {
    auto *editor = currentEditor();
    if (!editor) return;

    auto *container = m_tabManager->containerFor(editor);
    if (container && container->isFindBarVisible()) {
        container->findBar()->findNext();
        return;
    }

    if (!m_lastSearchText.isEmpty()) {
        performSearch(m_lastSearchText, m_lastUseRegex,
                      m_lastMatchCase, m_lastMatchWholeWord, true);
    } else if (container) {
        container->showFindBar();
    }
}

void SearchManager::findPrevious() {
    auto *editor = currentEditor();
    if (!editor) return;

    auto *container = m_tabManager->containerFor(editor);
    if (container && container->isFindBarVisible()) {
        container->findBar()->findPrevious();
        return;
    }

    if (!m_lastSearchText.isEmpty()) {
        performSearch(m_lastSearchText, m_lastUseRegex,
                      m_lastMatchCase, m_lastMatchWholeWord, false);
    } else if (container) {
        container->showFindBar();
    }
}

void SearchManager::performSearch(const QString &text, bool regex,
                                   bool caseSensitive, bool wholeWord,
                                   bool searchForward) {
    auto *editor = currentEditor();
    if (!editor || text.isEmpty()) return;

    bool found = editor->findFirst(text, regex, caseSensitive, wholeWord,
                                   true, searchForward);
    if (!found && m_parent && m_parent->statusBar()) {
        m_parent->statusBar()->showMessage(tr("Text not found"), 3000);
    }
}
