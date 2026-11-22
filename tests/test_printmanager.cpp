#include "codeeditor.h"
#include "plugins/pluginhost.h"
#include "plugins/print_plugin/printmanager.h"
#include "plugins/printinterface.h"
#include "settingsmanager.h"

#include <QFileInfo>
#include <QLibrary>
#include <QPrinter>
#include <QTemporaryDir>
#include <QTextDocument>

#include <gtest/gtest.h>

namespace
{
// The Print plugin is a runtime module; load it once and drive it through
// IPrintService, exactly as MainWindow does.
class PrintServiceHolder
{
public:
    PrintServiceHolder()
    {
#if defined(DEVPAD_BUILD_DIR) && defined(DEVPAD_BUILD_PLUGINS)
        m_library = std::make_unique<QLibrary>(QStringLiteral(DEVPAD_BUILD_DIR "/plugins/libprint_plugin" PLUGIN_SUFFIX));
        if (!m_library->load())
            return;
        auto* factory = reinterpret_cast<IPlugin* (*)()>(m_library->resolve("devpadCreatePlugin"));
        if (!factory)
            return;
        m_plugin = factory();
        m_host = std::make_unique<PluginHost>(nullptr);
        if (m_plugin->initialize(m_host.get()))
            m_service = dynamic_cast<IPrintService*>(m_host->object(QStringLiteral(DEVPAD_PRINT_SERVICE_OBJECT)));
#endif
    }

    ~PrintServiceHolder()
    {
        if (m_plugin)
        {
            m_plugin->shutdown();
            delete m_plugin;
        }
        // m_host is destroyed before m_library (reverse declaration order),
        // so no plugin code runs after the .so is unloaded.
    }

    IPrintService* service() const
    {
        return m_service;
    }

private:
    std::unique_ptr<QLibrary> m_library;
    std::unique_ptr<PluginHost> m_host;
    IPlugin* m_plugin = nullptr;
    IPrintService* m_service = nullptr;
};
} // namespace

class PrintManagerTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        ASSERT_TRUE(m_tempDir.isValid());
        m_testSettings = SettingsManager::createForTesting();
        SettingsManager::setTestingInstance(m_testSettings.get());
        m_editor = new CodeEditor();

        // Drive the printing code from a PrintManager linked into the test
        // binary. Loading the plugin's copy on Windows would run
        // qobject_cast<CodeEditor*> against CodeEditor::staticMetaObject from
        // Devpad_host.dll, while the editor here uses the statically linked
        // host copy; the cast fails and no PDF is ever written.
        m_service = std::make_unique<PrintManager>();
    }

    void configurePdfPrinter(QPrinter& printer) const
    {
        printer.setOutputFormat(QPrinter::PdfFormat);
        printer.setOutputFileName(m_tempDir.filePath("test_output.pdf"));
    }

    void TearDown() override
    {
        delete m_editor;
        m_service.reset();
        SettingsManager::setTestingInstance(nullptr);
        m_testSettings.reset();
    }

    std::unique_ptr<PrintManager> m_service;
    CodeEditor* m_editor = nullptr;
    std::unique_ptr<SettingsManager> m_testSettings;
    QTemporaryDir m_tempDir;
};

TEST_F(PrintManagerTest, PluginExposesPrintService)
{
#if defined(DEVPAD_BUILD_DIR) && defined(DEVPAD_BUILD_PLUGINS)
    PrintServiceHolder holder;
    EXPECT_NE(holder.service(), nullptr);
#else
    GTEST_SKIP() << "Print plugin not built";
#endif
}

TEST_F(PrintManagerTest, PrintWithNoLexerUsesPlainText)
{
    m_editor->setText("plain text content");
    m_editor->setSyntax("");

    QPrinter printer(QPrinter::HighResolution);
    configurePdfPrinter(printer);
    EXPECT_NO_THROW(m_service->printEditor(m_editor, &printer));
    EXPECT_TRUE(QFileInfo::exists(m_tempDir.filePath("test_output.pdf")));
}

TEST_F(PrintManagerTest, PrintWithCppLexerDoesNotCrash)
{
    m_editor->setText("int main() { return 0; }");
    m_editor->setSyntax("cpp");
    ASSERT_NE(m_editor->lexer(), nullptr);

    QPrinter printer(QPrinter::HighResolution);
    configurePdfPrinter(printer);
    EXPECT_NO_THROW(m_service->printEditor(m_editor, &printer));
    EXPECT_TRUE(QFileInfo::exists(m_tempDir.filePath("test_output.pdf")));
}

TEST_F(PrintManagerTest, PrintWithPythonLexerDoesNotCrash)
{
    m_editor->setText("def foo():\n    pass");
    m_editor->setSyntax("python");
    ASSERT_NE(m_editor->lexer(), nullptr);

    QPrinter printer(QPrinter::HighResolution);
    configurePdfPrinter(printer);
    EXPECT_NO_THROW(m_service->printEditor(m_editor, &printer));
    EXPECT_TRUE(QFileInfo::exists(m_tempDir.filePath("test_output.pdf")));
}

TEST_F(PrintManagerTest, PrintWithMultiLineDoesNotCrash)
{
    m_editor->setText("line one\nline two\nline three\nline four\nline five\n");
    m_editor->setSyntax("cpp");

    QPrinter printer(QPrinter::HighResolution);
    configurePdfPrinter(printer);
    EXPECT_NO_THROW(m_service->printEditor(m_editor, &printer));
    EXPECT_TRUE(QFileInfo::exists(m_tempDir.filePath("test_output.pdf")));
}

TEST_F(PrintManagerTest, PrintEmptyEditorDoesNotCrash)
{
    m_editor->setText("");
    m_editor->setSyntax("cpp");

    QPrinter printer(QPrinter::HighResolution);
    configurePdfPrinter(printer);
    EXPECT_NO_THROW(m_service->printEditor(m_editor, &printer));
    EXPECT_TRUE(QFileInfo::exists(m_tempDir.filePath("test_output.pdf")));
}

TEST_F(PrintManagerTest, EditorWithoutSyntaxHasNoLexer)
{
    m_editor->setText("plain text");
    m_editor->setSyntax("");
    EXPECT_EQ(m_editor->lexer(), nullptr);
}
