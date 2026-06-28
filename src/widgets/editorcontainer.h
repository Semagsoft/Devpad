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
    QVBoxLayout *m_layout;
    InlineFindBar *m_findBar;
    CodeEditor *m_editor;
};

#endif // EDITORCONTAINER_H
