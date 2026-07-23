#include "codeeditor.h"
#include "widgets/editorcontainer.h"
#include "panels/splitview.h"

#include <QSignalSpy>
#include <QTabWidget>
#include <gtest/gtest.h>

static const int PANE_TIMEOUT = 200;

TEST(SplitViewTest, InitialState)
{
    SplitView splitView;
    EXPECT_EQ(splitView.paneCount(), 1);
    EXPECT_NE(splitView.primaryTabWidget(), nullptr);
    EXPECT_NE(splitView.activeTabWidget(), nullptr);
}

TEST(SplitViewTest, AddNewPaneHorizontal)
{
    SplitView splitView;
    QTabWidget* primary = splitView.primaryTabWidget();

    QTabWidget* newPane = splitView.addNewPane(primary, Qt::Horizontal, true);
    EXPECT_NE(newPane, nullptr);
    EXPECT_EQ(splitView.paneCount(), 2);
}

TEST(SplitViewTest, AddNewPaneVertical)
{
    SplitView splitView;
    QTabWidget* primary = splitView.primaryTabWidget();

    QTabWidget* newPane = splitView.addNewPane(primary, Qt::Vertical, true);
    EXPECT_NE(newPane, nullptr);
    EXPECT_EQ(splitView.paneCount(), 2);
}

TEST(SplitViewTest, RemovePane)
{
    SplitView splitView;
    QTabWidget* primary = splitView.primaryTabWidget();

    QTabWidget* newPane = splitView.addNewPane(primary, Qt::Horizontal, true);
    ASSERT_EQ(splitView.paneCount(), 2);

    splitView.removePane(newPane);
    EXPECT_EQ(splitView.paneCount(), 1);
}

TEST(SplitViewTest, RemovePrimaryPaneDoesNotCrash)
{
    SplitView splitView;
    QTabWidget* primary = splitView.primaryTabWidget();

    splitView.addNewPane(primary, Qt::Horizontal, true);
    ASSERT_EQ(splitView.paneCount(), 2);

    splitView.removePane(primary);
    EXPECT_EQ(splitView.paneCount(), 1);
}

TEST(SplitViewTest, PaneAt)
{
    SplitView splitView;
    QTabWidget* primary = splitView.primaryTabWidget();

    EXPECT_EQ(splitView.paneAt(0), primary);
    EXPECT_EQ(splitView.paneAt(-1), nullptr);
    EXPECT_EQ(splitView.paneAt(1), nullptr);
}

TEST(SplitViewTest, ActiveTabWidgetChanges)
{
    SplitView splitView;
    QTabWidget* primary = splitView.primaryTabWidget();
    EXPECT_EQ(splitView.activeTabWidget(), primary);

    QTabWidget* newPane = splitView.addNewPane(primary, Qt::Horizontal, true);
    EXPECT_TRUE(splitView.activeTabWidget() == primary || splitView.activeTabWidget() == newPane);
}

TEST(SplitViewTest, MoveTabToPane)
{
    SplitView splitView;
    QTabWidget* pane1 = splitView.primaryTabWidget();

    auto* editor = new CodeEditor(&splitView);
    auto* container = new EditorContainer(editor, &splitView);
    pane1->addTab(container, QStringLiteral("Tab1"));
    ASSERT_EQ(pane1->count(), 1);

    QTabWidget* pane2 = splitView.addNewPane(pane1, Qt::Horizontal, true);
    ASSERT_EQ(splitView.paneCount(), 2);
    ASSERT_EQ(pane2->count(), 0);

    bool moved = splitView.moveTabToPane(0, pane1, pane2, 0);
    EXPECT_TRUE(moved);
    EXPECT_EQ(pane1->count(), 0);
    EXPECT_EQ(pane2->count(), 1);
}

TEST(SplitViewTest, DetachTabToWindow)
{
    SplitView splitView;
    QSignalSpy spy(&splitView, &SplitView::tabDetachedToWindow);

    QTabWidget* pane = splitView.primaryTabWidget();
    auto* editor = new CodeEditor(&splitView);
    editor->setFileName(QStringLiteral("/tmp/test_detach.txt"));
    auto* container = new EditorContainer(editor, &splitView);
    pane->addTab(container, QStringLiteral("test_detach.txt"));

    ASSERT_EQ(pane->count(), 1);
    splitView.detachTabToWindow(0, pane);

    EXPECT_EQ(spy.count(), 1);
    EXPECT_EQ(spy[0][0].toString(), QStringLiteral("/tmp/test_detach.txt"));
    EXPECT_EQ(pane->count(), 0);
}

TEST(SplitViewTest, DetachTabToWindowFailsForUntitled)
{
    SplitView splitView;
    QSignalSpy spy(&splitView, &SplitView::tabDetachedToWindow);

    QTabWidget* pane = splitView.primaryTabWidget();
    auto* editor = new CodeEditor(&splitView);
    auto* container = new EditorContainer(editor, &splitView);
    pane->addTab(container, QStringLiteral("Untitled"));

    splitView.detachTabToWindow(0, pane);
    EXPECT_EQ(spy.count(), 0);
}

TEST(SplitViewTest, SplitActivePaneHorizontal)
{
    SplitView splitView;

    splitView.splitActivePane(Qt::Horizontal);
    EXPECT_EQ(splitView.paneCount(), 2);
}

TEST(SplitViewTest, SplitActivePaneVertical)
{
    SplitView splitView;

    splitView.splitActivePane(Qt::Vertical);
    EXPECT_EQ(splitView.paneCount(), 2);
}

TEST(SplitViewTest, MaxPanesLimit)
{
    SplitView splitView;
    QTabWidget* primary = splitView.primaryTabWidget();

    QTabWidget* lastPane = primary;
    for (int i = 0; i < 4; ++i)
    {
        QTabWidget* newPane = splitView.addNewPane(lastPane, Qt::Horizontal, true);
        if (newPane)
            lastPane = newPane;
    }

    EXPECT_LE(splitView.paneCount(), 4);
}
