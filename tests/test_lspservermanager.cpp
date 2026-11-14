#include "lsp/lspservermanager.h"
#include "settingsmanager.h"

#include <gtest/gtest.h>

class LspServerManagerReplayTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        m_testSettings = SettingsManager::createForTesting();
        SettingsManager::setTestingInstance(m_testSettings.get());
        m_testSettings->setLspEnabled(true);
    }

    void TearDown() override
    {
        SettingsManager::setTestingInstance(nullptr);
        m_testSettings.reset();
    }

    std::unique_ptr<SettingsManager> m_testSettings;
};

// Use a language with no built-in server command so no process is spawned.
TEST_F(LspServerManagerReplayTest, TracksDocumentsWithoutServer)
{
    lsp::LspServerManager mgr;
    mgr.openDocument("cobol", "file:///a.cob", "first\n");
    mgr.openDocument("cobol", "file:///b.cob", "second\n");

    auto docs = mgr.documentsForReplay("cobol");
    ASSERT_EQ(docs.size(), 2);
    EXPECT_EQ(docs.first().first, "file:///a.cob");
    EXPECT_EQ(docs.first().second.first, "first\n");
    EXPECT_EQ(docs.first().second.second, 1);

    ASSERT_EQ(mgr.documentsForReplay("python").size(), 0);
}

TEST_F(LspServerManagerReplayTest, ChangeDocumentBumpsVersionAndText)
{
    lsp::LspServerManager mgr;
    mgr.openDocument("cobol", "file:///a.cob", "first\n");
    mgr.changeDocument("file:///a.cob", "second\n");

    auto docs = mgr.documentsForReplay("cobol");
    ASSERT_EQ(docs.size(), 1);
    EXPECT_EQ(docs.first().second.first, "second\n");
    EXPECT_EQ(docs.first().second.second, 2);
}

TEST_F(LspServerManagerReplayTest, ReopeningSameDocumentBumpsVersion)
{
    lsp::LspServerManager mgr;
    mgr.openDocument("cobol", "file:///a.cob", "first\n");
    mgr.openDocument("cobol", "file:///a.cob", "reopened\n");

    auto docs = mgr.documentsForReplay("cobol");
    ASSERT_EQ(docs.size(), 1);
    EXPECT_EQ(docs.first().second.first, "reopened\n");
    EXPECT_EQ(docs.first().second.second, 2);
}

TEST_F(LspServerManagerReplayTest, CloseDocumentStopsTracking)
{
    lsp::LspServerManager mgr;
    mgr.openDocument("cobol", "file:///a.cob", "first\n");
    mgr.closeDocument("file:///a.cob");

    EXPECT_TRUE(mgr.documentsForReplay("cobol").isEmpty());
}
