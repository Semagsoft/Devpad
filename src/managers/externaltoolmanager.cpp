#include "externaltoolmanager.h"

#include "settingsmanager.h"

#include <QDialogButtonBox>
#include <QDir>
#include <QFileInfo>
#include <QFontDatabase>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QProcess>
#include <QPushButton>
#include <QTextEdit>
#include <QVBoxLayout>

ExternalToolManager::ExternalToolManager(QObject* parent) : QObject(parent)
{
}

QList<ExternalTool> ExternalToolManager::tools() const
{
    return m_tools;
}

void ExternalToolManager::setTools(const QList<ExternalTool>& tools)
{
    m_tools = tools;
    emit toolsChanged();
}

QString ExternalToolManager::resolveVariables(const QString& template_, const QString& filePath, const QString& projectDir,
                                              const QString& selectedText, int lineNumber) const
{
    QString result = template_;

    QFileInfo fi(filePath);
    QString baseName = fi.completeBaseName();
    QString dir = fi.absolutePath();
    QString ext = fi.suffix();
    if (!ext.isEmpty())
        ext = "." + ext;

    result.replace("%f", filePath);
    result.replace("%n", baseName);
    result.replace("%d", dir);
    result.replace("%e", ext);
    result.replace("%p", projectDir);
    result.replace("%s", selectedText);
    result.replace("%l", QString::number(lineNumber));
    result.replace("%%", "%");

    return result;
}

QString ExternalToolManager::workingDirForTool(const ExternalTool& tool, const QString& filePath, const QString& projectDir) const
{
    QString wd = resolveVariables(tool.workingDir, filePath, projectDir, QString(), 0);
    if (wd.isEmpty())
    {
        if (!projectDir.isEmpty())
            wd = projectDir;
        else if (!filePath.isEmpty())
            wd = QFileInfo(filePath).absolutePath();
        else
            wd = QDir::homePath();
    }
    return wd;
}

QStringList ExternalToolManager::parseArguments(const QString& args)
{
    QStringList result;
    QString current;
    bool inSingleQuote = false;
    bool inDoubleQuote = false;

    for (int i = 0; i < args.size(); ++i)
    {
        const QChar c = args[i];

        if (inSingleQuote)
        {
            if (c == QLatin1Char('\''))
            {
                inSingleQuote = false;
            }
            else
            {
                current += c;
            }
        }
        else if (inDoubleQuote)
        {
            if (c == QLatin1Char('"'))
            {
                inDoubleQuote = false;
            }
            else if (c == QLatin1Char('\\') && i + 1 < args.size())
            {
                current += args[++i];
            }
            else
            {
                current += c;
            }
        }
        else
        {
            if (c == QLatin1Char('\''))
            {
                inSingleQuote = true;
            }
            else if (c == QLatin1Char('"'))
            {
                inDoubleQuote = true;
            }
            else if (c == QLatin1Char('\\') && i + 1 < args.size())
            {
                current += args[++i];
            }
            else if (c.isSpace())
            {
                if (!current.isEmpty())
                {
                    result << current;
                    current.clear();
                }
            }
            else
            {
                current += c;
            }
        }
    }

    if (!current.isEmpty())
        result << current;

    return result;
}

QString ExternalToolManager::shellEscape(const QString& s)
{
    QString escaped = s;
    escaped.replace(QLatin1Char('\''), QStringLiteral("'\\''"));
    return QLatin1Char('\'') + escaped + QLatin1Char('\'');
}

bool ExternalToolManager::runTool(int index, const QString& filePath, const QString& projectDir, const QString& selectedText, int lineNumber,
                                  const std::function<void(const QString&)>& terminalSender, QWidget* parent)
{
    auto& s = SettingsManager::instance();
    if (index < 0 || index >= s.externalToolCount())
        return false;

    ExternalTool tool;
    tool.name = s.externalToolName(index);
    tool.command = s.externalToolCommand(index);
    tool.arguments = s.externalToolArguments(index);
    tool.workingDir = s.externalToolWorkingDir(index);
    tool.shortcut = QKeySequence(s.externalToolShortcut(index));
    tool.runInTerminal = s.externalToolRunInTerminal(index);

    // Resolve variables in command
    QString resolvedCmd = resolveVariables(tool.command, filePath, projectDir, selectedText, lineNumber);

    // Parse arguments template first, then resolve variables in each token
    // This preserves argument boundaries when resolved values contain spaces
    QStringList parsedArgTemplates = parseArguments(tool.arguments);
    QStringList resolvedArgs;
    for (const auto& argTemplate : parsedArgTemplates)
    {
        resolvedArgs << resolveVariables(argTemplate, filePath, projectDir, selectedText, lineNumber);
    }

    QString resolvedWorkDir = workingDirForTool(tool, filePath, projectDir);

    if (tool.runInTerminal && terminalSender)
    {
        // Terminal path: build a properly shell-escaped command
        QString cmdLine = QStringLiteral("cd %1 && exec %2").arg(shellEscape(resolvedWorkDir), shellEscape(resolvedCmd));
        for (const auto& arg : resolvedArgs)
        {
            cmdLine += QLatin1Char(' ') + shellEscape(arg);
        }
        cmdLine += QLatin1Char('\n');
        terminalSender(cmdLine);
    }
    else
    {
        // Background QProcess path with output dialog
        auto* process = new QProcess(parent);
        process->setWorkingDirectory(resolvedWorkDir);
        process->setProcessChannelMode(QProcess::MergedChannels);

        auto* outputDialog = new QDialog(parent);
        outputDialog->setWindowTitle(tool.name + ExternalToolManager::tr(" - Output"));
        outputDialog->setMinimumSize(500, 300);
        auto* layout = new QVBoxLayout(outputDialog);
        auto* outputText = new QTextEdit(outputDialog);
        outputText->setReadOnly(true);
        outputText->setFont(QFontDatabase::systemFont(QFontDatabase::FixedFont));
        layout->addWidget(outputText);
        auto* closeBtn = new QPushButton(ExternalToolManager::tr("Close"), outputDialog);
        layout->addWidget(closeBtn);

        QObject::connect(closeBtn, &QPushButton::clicked, outputDialog, &QDialog::accept);
        QObject::connect(process, &QProcess::readyReadStandardOutput, outputDialog,
                         [process, outputText]() { outputText->append(QString::fromLocal8Bit(process->readAllStandardOutput())); });
        QObject::connect(process, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished), outputDialog,
                         [outputText, process](int, QProcess::ExitStatus)
                         {
                             outputText->append(ExternalToolManager::tr("\n--- Process exited ---"));
                             outputText->append(QString::fromLocal8Bit(process->readAllStandardOutput()));
                         });
        outputDialog->setAttribute(Qt::WA_DeleteOnClose);
        outputDialog->show();

        process->start(resolvedCmd, resolvedArgs);
    }

    return true;
}

ExternalToolEditDialog::ExternalToolEditDialog(QWidget* parent) : QDialog(parent)
{
    setupUI();
}

ExternalToolEditDialog::ExternalToolEditDialog(const ExternalTool& tool, QWidget* parent) : QDialog(parent)
{
    setupUI();
    setTool(tool);
}

ExternalTool ExternalToolEditDialog::tool() const
{
    ExternalTool t;
    t.name = m_nameEdit->text().trimmed();
    t.command = m_commandEdit->text().trimmed();
    t.arguments = m_argumentsEdit->text().trimmed();
    t.workingDir = m_workDirEdit->text().trimmed();
    t.shortcut = m_shortcutEdit->keySequence();
    t.runInTerminal = m_runInTerminalCheck->isChecked();
    return t;
}

void ExternalToolEditDialog::setTool(const ExternalTool& tool)
{
    m_nameEdit->setText(tool.name);
    m_commandEdit->setText(tool.command);
    m_argumentsEdit->setText(tool.arguments);
    m_workDirEdit->setText(tool.workingDir);
    m_shortcutEdit->setKeySequence(tool.shortcut);
    m_runInTerminalCheck->setChecked(tool.runInTerminal);
}

void ExternalToolEditDialog::setupUI()
{
    setWindowTitle(tr("Edit External Tool"));
    setMinimumWidth(450);

    auto* mainLayout = new QVBoxLayout(this);

    auto* formLayout = new QFormLayout();

    m_nameEdit = new QLineEdit(this);
    m_nameEdit->setPlaceholderText(tr("My Tool"));
    formLayout->addRow(tr("Name:"), m_nameEdit);

    m_commandEdit = new QLineEdit(this);
    m_commandEdit->setPlaceholderText(tr("python3, gcc, make, ..."));
    formLayout->addRow(tr("Command:"), m_commandEdit);

    m_argumentsEdit = new QLineEdit(this);
    m_argumentsEdit->setPlaceholderText(tr("%f, %n, %d, %p, %e, %s, %l, %%"));
    formLayout->addRow(tr("Arguments:"), m_argumentsEdit);

    m_workDirEdit = new QLineEdit(this);
    m_workDirEdit->setPlaceholderText(tr("Leave empty for file/project directory"));
    formLayout->addRow(tr("Working dir:"), m_workDirEdit);

    m_shortcutEdit = new QKeySequenceEdit(this);
    formLayout->addRow(tr("Shortcut:"), m_shortcutEdit);

    m_runInTerminalCheck = new QCheckBox(tr("Run in terminal panel"), this);
    m_runInTerminalCheck->setChecked(true);
    formLayout->addRow(m_runInTerminalCheck);

    mainLayout->addLayout(formLayout);

    m_hintLabel = new QLabel(tr("Variables: %f = full path, %n = filename, %d = directory, "
                                "%p = project dir, %e = extension, %s = selected text, "
                                "%l = line number, %% = literal %"),
                             this);
    m_hintLabel->setWordWrap(true);
    m_hintLabel->setStyleSheet("color: gray; font-size: 11px;");
    mainLayout->addWidget(m_hintLabel);

    auto* buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    connect(buttonBox, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
    mainLayout->addWidget(buttonBox);
}
