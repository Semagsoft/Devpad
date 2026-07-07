#ifndef EDITORCONTAINER_H
#define EDITORCONTAINER_H

#include <QWidget>
#include <QString>

class QVBoxLayout;
class CodeEditor;
class InlineFindBar;

class EditorContainer : public QWidget {
    Q_OBJECT

public:
    explicit EditorContainer(CodeEditor *editor, QWidget *parent = nullptr);

    CodeEditor *editor() const { return m_editor; }
    InlineFindBar *findBar() const { return m_findBar; }

    void showFindBar();
    void showReplaceBar();
    void hideFindBar();
    bool isFindBarVisible() const;

private:
    QVBoxLayout *m_layout = nullptr;
    InlineFindBar *m_findBar = nullptr;
    CodeEditor *m_editor = nullptr;
};

#endif // EDITORCONTAINER_H
