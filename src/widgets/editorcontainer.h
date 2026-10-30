#ifndef EDITORCONTAINER_H
#define EDITORCONTAINER_H

#include <QString>
#include <QWidget>

class QVBoxLayout;
class QHBoxLayout;
class CodeEditor;
class InlineFindBar;
class Minimap;

class EditorContainer : public QWidget
{
    Q_OBJECT

public:
    explicit EditorContainer(CodeEditor* editor, QWidget* parent = nullptr);
    ~EditorContainer() override = default;

    CodeEditor* editor() const
    {
        return m_editor;
    }
    InlineFindBar* findBar() const
    {
        return m_findBar;
    }
    Minimap* minimap() const
    {
        return m_minimap;
    }

    void showFindBar();
    void showReplaceBar();
    void hideFindBar();
    bool isFindBarVisible() const;

    // Show/hide the minimap strip for this editor.
    void setMinimapVisible(bool visible);
    bool isMinimapVisible() const;

private:
    void applyMinimapSetting();

    QVBoxLayout* m_centerLayout = nullptr; // find bar + editor (center column)
    QHBoxLayout* m_layout = nullptr;       // center column + minimap strip
    InlineFindBar* m_findBar = nullptr;
    CodeEditor* m_editor = nullptr;
    Minimap* m_minimap = nullptr;
};

#endif // EDITORCONTAINER_H
