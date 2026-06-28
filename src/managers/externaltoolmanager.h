#ifndef EXTERNALTOOLMANAGER_H
#define EXTERNALTOOLMANAGER_H

#include <QObject>
#include <QString>
#include <QStringList>
#include <QKeySequence>
#include <QList>
#include <QDialog>
#include <QLineEdit>
#include <QCheckBox>
#include <QKeySequenceEdit>
#include <QLabel>
#include <functional>

struct ExternalTool {
    QString name;
    QString command;
    QString arguments;
    QString workingDir;
    QKeySequence shortcut;
    bool runInTerminal = true;
};

class ExternalToolManager : public QObject {
    Q_OBJECT

public:
    explicit ExternalToolManager(QObject *parent = nullptr);

    QList<ExternalTool> tools() const;
    void setTools(const QList<ExternalTool>& tools);

    QString resolveVariables(const QString& template_, const QString& filePath,
                             const QString& projectDir, const QString& selectedText,
                             int lineNumber) const;
    QString workingDirForTool(const ExternalTool& tool, const QString& filePath,
                              const QString& projectDir) const;

    // Parse arguments string into individual tokens, respecting shell quoting
    static QStringList parseArguments(const QString& args);

    // Escape a string for safe use in a POSIX shell (single-quote wrapping)
    static QString shellEscape(const QString& s);

    // Run external tool by index.
    // For terminal tools, the escaped shell command is passed via terminalSender.
    // For background tools, a QProcess + output dialog is created.
    // Returns true if the index was valid.
    bool runTool(int index, const QString& filePath, const QString& projectDir,
                 const QString& selectedText, int lineNumber,
                 const std::function<void(const QString&)>& terminalSender,
                 QWidget* parent = nullptr);

signals:
    void toolsChanged();

private:
    QList<ExternalTool> m_tools;
};

class ExternalToolEditDialog : public QDialog {
    Q_OBJECT

public:
    explicit ExternalToolEditDialog(QWidget *parent = nullptr);
    explicit ExternalToolEditDialog(const ExternalTool& tool, QWidget *parent = nullptr);

    ExternalTool tool() const;
    void setTool(const ExternalTool& tool);

private:
    void setupUI();

    QLineEdit* m_nameEdit;
    QLineEdit* m_commandEdit;
    QLineEdit* m_argumentsEdit;
    QLineEdit* m_workDirEdit;
    QKeySequenceEdit* m_shortcutEdit;
    QCheckBox* m_runInTerminalCheck;
    QLabel* m_hintLabel;
};

#endif
