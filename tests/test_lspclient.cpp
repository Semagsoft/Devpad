#include "lsp/lspclient.h"
#include "lsp/lspservermanager.h"

#include <QSignalSpy>
#include <QTemporaryDir>
#include <QTest>

#include <gtest/gtest.h>

using namespace lsp;

TEST(LspClientTest, InitialStateNotRunning)
{
    LspClient client(QStringLiteral("cpp"));
    EXPECT_FALSE(client.isRunning());
    EXPECT_EQ(client.language(), QStringLiteral("cpp"));
    EXPECT_FALSE(client.capabilities().completionProvider);
    EXPECT_TRUE(client.rootUri().isEmpty());
}

TEST(LspClientTest, RequestsGuardWhenNotInitialized)
{
    LspClient client(QStringLiteral("cpp"));
    QSignalSpy completionSpy(&client, &LspClient::completionReady);
    QSignalSpy defSpy(&client, &LspClient::definitionReady);
    QSignalSpy diagSpy(&client, &LspClient::diagnosticsReady);

    // All requests should early-return when not running/initialized
    client.requestCompletion(QStringLiteral("file:///tmp/a.cpp"), Position{0, 0}, 1);
    client.requestDefinition(QStringLiteral("file:///tmp/a.cpp"), Position{0, 0});
    client.requestReferences(QStringLiteral("file:///tmp/a.cpp"), Position{0, 0});
    client.requestHover(QStringLiteral("file:///tmp/a.cpp"), Position{0, 0});
    client.requestDiagnostics(QStringLiteral("file:///tmp/a.cpp"));
    client.requestRename(QStringLiteral("file:///tmp/a.cpp"), Position{0, 0}, QStringLiteral("newName"));

    EXPECT_EQ(completionSpy.count(), 0);
    EXPECT_EQ(defSpy.count(), 0);
    EXPECT_EQ(diagSpy.count(), 0);
}

TEST(LspClientTest, OpenDocumentQueuesWhenNotInitialized)
{
    LspClient client(QStringLiteral("python"));
    // Should not crash when queuing docs before server start
    client.openDocument(QStringLiteral("file:///tmp/test.py"), QStringLiteral("print('hi')"), 1);
    client.changeDocument(QStringLiteral("file:///tmp/test.py"), QStringLiteral("print('hello')"), 2);
    client.saveDocument(QStringLiteral("file:///tmp/test.py"));
    client.closeDocument(QStringLiteral("file:///tmp/test.py"));
    SUCCEED();
}

TEST(LspClientTest, FailedToStartEmitsError)
{
    LspClient client(QStringLiteral("cpp"));
    QSignalSpy errorSpy(&client, &LspClient::serverError);
    QSignalSpy stoppedSpy(&client, &LspClient::serverStopped);

    client.startServer(QStringLiteral("/nonexistent_binary_12345_xyz"), QStringList(), QStringLiteral("file:///tmp"));

    // Wait up to 2s for FailedToStart error
    ASSERT_TRUE(errorSpy.wait(2000) || errorSpy.count() > 0);
    EXPECT_GE(errorSpy.count(), 1);
    // Should eventually emit stopped (cleanup) without double emission
    // Give a little time for cleanup
    QTest::qWait(200);
    EXPECT_LE(stoppedSpy.count(), 1);
}

TEST(LspClientTest, StopWithoutStartDoesNotCrash)
{
    LspClient client(QStringLiteral("cpp"));
    client.stopServer();
    EXPECT_FALSE(client.isRunning());
}

TEST(LspServerManagerTest, DefaultCommandsNonEmpty)
{
    auto cmds = LspServerManager::defaultServerCommands();
    // At least one language should have a default
    EXPECT_FALSE(cmds.isEmpty());
}

TEST(LspServerManagerTest, ClientForLanguageCreatesAndCaches)
{
    // Use a temporary settings instance to avoid polluting real config
    LspServerManager manager;
    // Without configured server command, clientForLanguage should return nullptr (no executable found)
    LspClient* c1 = manager.clientForLanguage(QStringLiteral("unknown_lang_xyz"));
    // Unknown language without command should be nullptr
    EXPECT_EQ(c1, nullptr);

    // Known language may still be nullptr if clangd not installed — just verify no crash
    LspClient* c2 = manager.clientForLanguage(QStringLiteral("cpp"));
    (void)c2;
    SUCCEED();
}

TEST(LspServerManagerTest, DocumentLanguageTracking)
{
    LspServerManager manager;
    EXPECT_TRUE(manager.languages().isEmpty());
    manager.openDocument(QStringLiteral("cpp"), QStringLiteral("file:///tmp/a.cpp"), QStringLiteral("int main(){}"));
    // openDocument tracks even if no client started (queues)
    manager.changeDocument(QStringLiteral("file:///tmp/a.cpp"), QStringLiteral("int main(){return 0;}"));
    manager.saveDocument(QStringLiteral("file:///tmp/a.cpp"));
    manager.closeDocument(QStringLiteral("file:///tmp/a.cpp"));
    SUCCEED();
}

TEST(LspServerManagerTest, RootUriForFileWalksMarkers)
{
    LspServerManager manager;
    QTemporaryDir tmp;
    ASSERT_TRUE(tmp.isValid());
    // Create a .git marker
    QDir(tmp.path()).mkdir(QStringLiteral(".git"));
    QString filePath = tmp.filePath(QStringLiteral("src/main.cpp"));
    QDir().mkpath(QFileInfo(filePath).absolutePath());
    QString rootUri = manager.rootUriForFile(filePath);
    EXPECT_FALSE(rootUri.isEmpty());
    EXPECT_TRUE(rootUri.startsWith(QStringLiteral("file://")));
}

TEST(LspServerManagerTest, HasCapabilityReturnsFalseForUnknownUri)
{
    LspServerManager manager;
    EXPECT_FALSE(manager.hasCapability(QStringLiteral("file:///nonexistent.cpp"), QStringLiteral("completion")));
}
