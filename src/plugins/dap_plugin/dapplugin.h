#ifndef DAPPLUGIN_H
#define DAPPLUGIN_H

#include "plugins/plugininterface.h"

#include <QObject>
#include <QStringList>

// DAP debugger plugin. Provides Debug Adapter Protocol debugging support.
// Creates DapServerManager, DebugPanel, and per-editor DapEditorIntegration
// instances. Communicates with MainWindow via PluginHost::registerObject()
// and PluginHost::panel().
class CodeEditor;
class DebugPanel;

namespace dap
{
class DapServerManager;
class DapEditorIntegration;
class Capabilities;
}

class DapPlugin : public QObject, public IPlugin
{
    Q_OBJECT

public:
    explicit DapPlugin(QObject* parent = nullptr);
    ~DapPlugin() override;

    PluginInfo info() const override;
    bool initialize(IPluginHost* host) override;
    void shutdown() override;

    // ── Debug session control (Q_INVOKABLE for MainWindow) ──────────
    Q_INVOKABLE void startDebugging(const QString& sessionKey, const QString& language,
                                    const QString& command, const QStringList& args);
    Q_INVOKABLE void stopDebugging(const QString& sessionKey);
    Q_INVOKABLE void debugContinue(const QString& sessionKey);
    Q_INVOKABLE void debugStepOver(const QString& sessionKey);
    Q_INVOKABLE void debugStepInto(const QString& sessionKey);
    Q_INVOKABLE void debugStepOut(const QString& sessionKey);
    Q_INVOKABLE void debugRestart(const QString& sessionKey);

    // ── Editor integration ────────────────────────────────────────
    // Called by PluginManager when a new editor is created.
    void onEditorCreated(CodeEditor* editor);

private slots:
    void onInitializeDone(const QString& sessionKey, const dap::Capabilities& caps);
    void onClientStopped(const QString& sessionKey);

private:
    dap::DapServerManager* m_dapServerManager = nullptr;
    DebugPanel* m_debugPanel = nullptr;
    IPluginHost* m_host = nullptr;

    // Track editor integrations so we can set session key on all of them.
    QList<dap::DapEditorIntegration*> m_editorIntegrations;

    QString m_debugSessionKey;
    QString m_debugLanguage;
    QString m_debugCommand;
    QStringList m_debugArgs;
};

#endif // DAPPLUGIN_H
