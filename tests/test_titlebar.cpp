#include "managers/settingsmanager.h"
#include "widgets/titlebar.h"

#include <QApplication>
#include <QLabel>
#include <QLayout>
#include <QMainWindow>
#include <QMenuBar>
#include <QSignalSpy>
#include <QToolButton>
#include <memory>

#include <gtest/gtest.h>

class TitleBarTest : public ::testing::Test
{
protected:
    std::unique_ptr<SettingsManager> m_testSettings;

    void SetUp() override
    {
        m_testSettings = SettingsManager::createForTesting();
        SettingsManager::setTestingInstance(m_testSettings.get());
    }

    void TearDown() override
    {
        SettingsManager::setTestingInstance(nullptr);
        m_testSettings.reset();
    }
};

TEST_F(TitleBarTest, WindowButtonsEmitSignals)
{
    TitleBar titleBar;
    QSignalSpy minSpy(&titleBar, &TitleBar::minimizeRequested);
    QSignalSpy maxSpy(&titleBar, &TitleBar::maximizeRequested);
    QSignalSpy closeSpy(&titleBar, &TitleBar::closeRequested);

    titleBar.minimizeButton()->click();
    titleBar.maximizeButton()->click();
    titleBar.closeButton()->click();

    EXPECT_EQ(minSpy.count(), 1);
    EXPECT_EQ(maxSpy.count(), 1);
    EXPECT_EQ(closeSpy.count(), 1);
}

TEST_F(TitleBarTest, ButtonsArePresent)
{
    TitleBar titleBar;
    EXPECT_NE(titleBar.minimizeButton(), nullptr);
    EXPECT_NE(titleBar.maximizeButton(), nullptr);
    EXPECT_NE(titleBar.closeButton(), nullptr);
    EXPECT_EQ(titleBar.minimizeButton()->parentWidget(), &titleBar);
    EXPECT_EQ(titleBar.maximizeButton()->parentWidget(), &titleBar);
    EXPECT_EQ(titleBar.closeButton()->parentWidget(), &titleBar);
}

TEST_F(TitleBarTest, MaximizedTogglesRestoreTooltip)
{
    TitleBar titleBar;
    titleBar.setMaximized(false);
    EXPECT_EQ(titleBar.maximizeButton()->toolTip(), QString("Maximize"));

    titleBar.setMaximized(true);
    EXPECT_EQ(titleBar.maximizeButton()->toolTip(), QString("Restore"));

    titleBar.setMaximized(false);
    EXPECT_EQ(titleBar.maximizeButton()->toolTip(), QString("Maximize"));
}

TEST_F(TitleBarTest, SetMenuBarReparentsAndInserts)
{
    TitleBar titleBar;
    auto* menuBar = new QMenuBar();
    titleBar.setMenuBar(menuBar);

    EXPECT_EQ(menuBar->parentWidget(), &titleBar);
    EXPECT_EQ(titleBar.menuBarWidget(), menuBar);
    EXPECT_GE(titleBar.layout()->indexOf(menuBar), 0);
}

TEST_F(TitleBarTest, SetTitleTextUpdatesLabel)
{
    TitleBar titleBar;
    titleBar.setTitleText(QStringLiteral("Hello"));
    auto* label = titleBar.findChild<QLabel*>("TitleBarLabel");
    ASSERT_NE(label, nullptr);
    EXPECT_EQ(label->text(), QString("Hello"));
}

TEST_F(TitleBarTest, MenuInTitlebarSettingRoundTrip)
{
    EXPECT_TRUE(SettingsManager::instance().showMenuInTitlebar());

    SettingsManager::instance().setShowMenuInTitlebar(false);
    EXPECT_FALSE(SettingsManager::instance().showMenuInTitlebar());

    SettingsManager::instance().setShowMenuInTitlebar(true);
    EXPECT_TRUE(SettingsManager::instance().showMenuInTitlebar());
}

TEST_F(TitleBarTest, HostsInMainWindowMenuBarSlot)
{
    QMainWindow window;
    window.resize(400, 300);
    window.setCentralWidget(new QWidget(&window));

    auto* titleBar = new TitleBar(&window);
    window.setMenuWidget(titleBar);

    auto* menuBar = new QMenuBar();
    titleBar->setMenuBar(menuBar);
    menuBar->addAction(QStringLiteral("File"));

    window.show();
    QApplication::processEvents();

    EXPECT_EQ(menuBar->parentWidget(), titleBar);
    EXPECT_EQ(titleBar->menuBarWidget(), menuBar);
    EXPECT_GE(titleBar->layout()->indexOf(menuBar), 0);
    EXPECT_TRUE(titleBar->isVisible());
    EXPECT_GT(titleBar->geometry().height(), 0);
    EXPECT_EQ(titleBar->geometry().top(), 0);
    EXPECT_EQ(window.menuWidget(), titleBar);

    window.setMenuBar(menuBar);
    QApplication::processEvents();
    EXPECT_EQ(menuBar->parentWidget(), &window);
    EXPECT_EQ(window.menuWidget(), menuBar);
    EXPECT_TRUE(menuBar->isVisible());
}
