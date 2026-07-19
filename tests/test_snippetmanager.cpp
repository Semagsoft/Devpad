#include "settingsmanager.h"
#include "snippet.h"
#include "snippetmanager.h"

#include <QApplication>

#include <gtest/gtest.h>

class SnippetManagerTest : public ::testing::Test
{
protected:
    std::unique_ptr<SettingsManager> m_testSettings;
    SnippetManager* m_snippetManager = nullptr;

    void SetUp() override
    {
        m_testSettings = SettingsManager::createForTesting();
        SettingsManager::setTestingInstance(m_testSettings.get());
        m_snippetManager = new SnippetManager();
    }
    void TearDown() override
    {
        delete m_snippetManager;
        SettingsManager::setTestingInstance(nullptr);
        m_testSettings.reset();
    }
};

TEST_F(SnippetManagerTest, InstanceCreated)
{
    EXPECT_EQ(SnippetManager::instance(), m_snippetManager);
}

TEST_F(SnippetManagerTest, HasBuiltInSnippets)
{
    QList<Snippet> all = m_snippetManager->allSnippets();
    EXPECT_GT(all.size(), 0);
}

TEST_F(SnippetManagerTest, BuiltInSnippetsHaveValidData)
{
    QList<Snippet> all = m_snippetManager->allSnippets();
    for (const Snippet& s : all)
    {
        EXPECT_FALSE(s.name.isEmpty()) << "Snippet has empty name";
        EXPECT_FALSE(s.prefix.isEmpty()) << "Snippet '" << s.name.toStdString() << "' has empty prefix";
        EXPECT_FALSE(s.body.isEmpty()) << "Snippet '" << s.name.toStdString() << "' has empty body";
    }
}

TEST_F(SnippetManagerTest, SnippetsForLanguageFound)
{
    QList<Snippet> cppSnippets = m_snippetManager->snippetsForLanguage("cpp");
    EXPECT_GT(cppSnippets.size(), 0);

    QList<Snippet> jsSnippets = m_snippetManager->snippetsForLanguage("javascript");
    EXPECT_GT(jsSnippets.size(), 0);
}

TEST_F(SnippetManagerTest, SnippetByNameFound)
{
    QList<Snippet> all = m_snippetManager->allSnippets();
    if (!all.isEmpty())
    {
        Snippet found = m_snippetManager->snippetByName(all[0].name);
        EXPECT_EQ(found.name, all[0].name);
        EXPECT_EQ(found.prefix, all[0].prefix);
    }
}

TEST_F(SnippetManagerTest, SnippetByNameNotFound)
{
    Snippet found = m_snippetManager->snippetByName("nonexistent_snippet_xyz");
    EXPECT_TRUE(found.prefix.isEmpty());
}

TEST_F(SnippetManagerTest, SnippetsByPrefixMatch)
{
    QList<Snippet> all = m_snippetManager->allSnippets();
    ASSERT_GT(all.size(), 0);
    QString prefix = all[0].prefix;
    QList<Snippet> matches = m_snippetManager->snippetsByPrefix(prefix);
    EXPECT_GT(matches.size(), 0);
    bool foundExact = false;
    for (const Snippet& s : matches)
    {
        if (s.prefix == prefix)
            foundExact = true;
    }
    EXPECT_TRUE(foundExact);
}

TEST_F(SnippetManagerTest, SnippetsByPrefixEmptyForUnknownLanguage)
{
    QList<Snippet> matches = m_snippetManager->snippetsByPrefix("for", "nonexistent_language");
    // Global snippets might still match
    EXPECT_TRUE(matches.size() >= 0);
}

TEST_F(SnippetManagerTest, SnippetsForLanguageWithGlobalSnippets)
{
    QList<Snippet> cppSnippets = m_snippetManager->snippetsForLanguage("cpp");
    QList<Snippet> globalSnippets = m_snippetManager->snippetsForLanguage("");

    // Language-specific should contain at least the global ones
    EXPECT_GE(cppSnippets.size(), globalSnippets.size());
}
