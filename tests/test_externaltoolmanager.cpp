#include "externaltoolmanager.h"
#include "settingsmanager.h"

#include <QFileInfo>

#include <gtest/gtest.h>

class ExternalToolManagerTest : public ::testing::Test
{
protected:
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

    ExternalToolManager manager;
    std::unique_ptr<SettingsManager> m_testSettings;
};

TEST_F(ExternalToolManagerTest, ResolveVariablesFullPath)
{
    EXPECT_EQ(manager.resolveVariables("%f", "/home/user/file.cpp", "/home/user/project", "selected", 10), "/home/user/file.cpp");
}

TEST_F(ExternalToolManagerTest, ResolveVariablesBaseName)
{
    EXPECT_EQ(manager.resolveVariables("%n", "/home/user/file.cpp", "", "", 0), "file");
}

TEST_F(ExternalToolManagerTest, ResolveVariablesDirectory)
{
    QFileInfo fi("/home/user/file.cpp");
    EXPECT_EQ(manager.resolveVariables("%d", "/home/user/file.cpp", "", "", 0), fi.absolutePath());
}

TEST_F(ExternalToolManagerTest, ResolveVariablesExtension)
{
    EXPECT_EQ(manager.resolveVariables("%e", "/home/user/file.cpp", "", "", 0), ".cpp");
}

TEST_F(ExternalToolManagerTest, ResolveVariablesProjectDir)
{
    EXPECT_EQ(manager.resolveVariables("%p", "", "/home/user/project", "", 0), "/home/user/project");
}

TEST_F(ExternalToolManagerTest, ResolveVariablesSelectedText)
{
    EXPECT_EQ(manager.resolveVariables("%s", "", "", "selected_text", 0), "selected_text");
}

TEST_F(ExternalToolManagerTest, ResolveVariablesLineNumber)
{
    EXPECT_EQ(manager.resolveVariables("%l", "", "", "", 42), "42");
}

TEST_F(ExternalToolManagerTest, ResolveVariablesPercentEscape)
{
    EXPECT_EQ(manager.resolveVariables("%%", "", "", "", 0), "%");
}

TEST_F(ExternalToolManagerTest, ResolveVariablesComplexTemplate)
{
    QFileInfo fi("/path/to/script.py");
    // %e expands with leading dot, so %n.%e → "script" + "." + ".py" = "script..py"
    EXPECT_EQ(manager.resolveVariables("%n at %d", "/path/to/script.py", "", "", 0), fi.completeBaseName() + " at " + fi.absolutePath());
}

TEST_F(ExternalToolManagerTest, ResolveVariablesExtensionEmptyWhenNoExt)
{
    EXPECT_EQ(manager.resolveVariables("%e", "/home/user/README", "", "", 0), "");
}

TEST_F(ExternalToolManagerTest, ParseArgumentsEmpty)
{
    EXPECT_TRUE(ExternalToolManager::parseArguments("").isEmpty());
}

TEST_F(ExternalToolManagerTest, ParseArgumentsSimple)
{
    QStringList result = ExternalToolManager::parseArguments("-a -b --long");
    ASSERT_EQ(result.size(), 3);
    EXPECT_EQ(result[0], "-a");
    EXPECT_EQ(result[1], "-b");
    EXPECT_EQ(result[2], "--long");
}

TEST_F(ExternalToolManagerTest, ParseArgumentsSingleQuotes)
{
    QStringList result = ExternalToolManager::parseArguments("echo 'hello world'");
    ASSERT_EQ(result.size(), 2);
    EXPECT_EQ(result[0], "echo");
    EXPECT_EQ(result[1], "hello world");
}

TEST_F(ExternalToolManagerTest, ParseArgumentsDoubleQuotes)
{
    QStringList result = ExternalToolManager::parseArguments("echo \"hello world\"");
    ASSERT_EQ(result.size(), 2);
    EXPECT_EQ(result[0], "echo");
    EXPECT_EQ(result[1], "hello world");
}

#ifndef Q_OS_WIN
TEST_F(ExternalToolManagerTest, ParseArgumentsBackslashEscape)
{
    QStringList result = ExternalToolManager::parseArguments("arg\\ with\\ spaces");
    ASSERT_EQ(result.size(), 1);
    EXPECT_EQ(result[0], "arg with spaces");
}
#else
TEST_F(ExternalToolManagerTest, ParseArgumentsWindowsBackslashPreserved)
{
    QStringList result = ExternalToolManager::parseArguments("--path C:\\Users\\primo\\file.cpp");
    ASSERT_EQ(result.size(), 2);
    EXPECT_EQ(result[1], "C:\\Users\\primo\\file.cpp");
}

TEST_F(ExternalToolManagerTest, ParseArgumentsWindowsQuotedPath)
{
    QStringList result = ExternalToolManager::parseArguments("\"C:\\Program Files\\app\\run.exe\" --flag");
    ASSERT_EQ(result.size(), 2);
    EXPECT_EQ(result[0], "C:\\Program Files\\app\\run.exe");
}

TEST_F(ExternalToolManagerTest, CmdQuoteWindows)
{
    EXPECT_EQ(ExternalToolManager::cmdQuote("C:\\Users\\primo\\My Documents"), "\"C:\\Users\\primo\\My Documents\"");
    EXPECT_EQ(ExternalToolManager::cmdQuote("a\"b"), "\"a\"\"b\"");
}
#endif

TEST_F(ExternalToolManagerTest, ParseArgumentsMixedQuotes)
{
    QStringList result = ExternalToolManager::parseArguments(R"(program 'single "quote"' "double 'quote'")");
    ASSERT_EQ(result.size(), 3);
    EXPECT_EQ(result[0], "program");
}

TEST_F(ExternalToolManagerTest, ShellEscapeSimple)
{
    EXPECT_EQ(ExternalToolManager::shellEscape("hello"), "'hello'");
}

TEST_F(ExternalToolManagerTest, ShellEscapeWithSingleQuote)
{
    EXPECT_EQ(ExternalToolManager::shellEscape("it's"), "'it'\\''s'");
}

TEST_F(ExternalToolManagerTest, ShellEscapeEmpty)
{
    EXPECT_EQ(ExternalToolManager::shellEscape(""), "''");
}

TEST_F(ExternalToolManagerTest, WorkingDirWithProjectDir)
{
    ExternalTool tool;
    tool.workingDir = "";
    QString result = manager.workingDirForTool(tool, "", "/project");
    EXPECT_EQ(result, "/project");
}

TEST_F(ExternalToolManagerTest, WorkingDirWithFilePath)
{
    ExternalTool tool;
    tool.workingDir = "";
    QString result = manager.workingDirForTool(tool, "/path/to/file.txt", "");
    EXPECT_EQ(result, QFileInfo("/path/to/file.txt").absolutePath());
}

TEST_F(ExternalToolManagerTest, WorkingDirFallbackToHome)
{
    ExternalTool tool;
    tool.workingDir = "";
    QString result = manager.workingDirForTool(tool, "", "");
    EXPECT_FALSE(result.isEmpty());
}

TEST_F(ExternalToolManagerTest, WorkingDirResolvedFromTemplate)
{
    ExternalTool tool;
    tool.workingDir = "%d";
    QString result = manager.workingDirForTool(tool, "/custom/dir/file.txt", "/project");
    EXPECT_EQ(result, QFileInfo("/custom/dir/file.txt").absolutePath());
}

TEST_F(ExternalToolManagerTest, SetToolsAndRetrieve)
{
    QList<ExternalTool> tools;
    ExternalTool t1;
    t1.name = "Build";
    t1.command = "make";
    tools.append(t1);
    ExternalTool t2;
    t2.name = "Run";
    t2.command = "./output";
    tools.append(t2);
    manager.setTools(tools);

    EXPECT_EQ(manager.tools().size(), 2);
    EXPECT_EQ(manager.tools()[0].name, "Build");
    EXPECT_EQ(manager.tools()[1].name, "Run");
}

TEST_F(ExternalToolManagerTest, RunToolInvalidIndex)
{
    bool result = manager.runTool(-1, "", "", "", 0, [](const QString&) {}, nullptr);
    EXPECT_FALSE(result);
}

TEST_F(ExternalToolManagerTest, RunToolOutOfBoundsIndex)
{
    bool result = manager.runTool(999, "", "", "", 0, [](const QString&) {}, nullptr);
    EXPECT_FALSE(result);
}
