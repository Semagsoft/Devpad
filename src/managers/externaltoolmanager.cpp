#include "externaltoolmanager.h"

#include "settingsmanager.h"

#include <QDialogButtonBox>
#include <QDir>
#include <QFileInfo>
#include <QFontDatabase>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QPointer>
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
    QFileInfo fi(filePath);
    QString baseName = fi.completeBaseName();
    QString dir = fi.absolutePath();
    QString ext = fi.suffix();
    if (!ext.isEmpty())
        ext = "." + ext;

    // Single left-to-right pass: substituted values are appended verbatim and
    // never rescanned by a later token. Sequential QString::replace() calls
    // would corrupt a value that itself contains a token (e.g. a path with
    // "%n") and could not implement "%%" as a literal percent.
    QString result;
    result.reserve(template_.size());
    for (int i = 0; i < template_.size();)
    {
        if (template_.at(i) != QLatin1Char('%'))
        {
            result.append(template_.at(i++));
            continue;
        }
        if (i + 1 >= template_.size())
        {
            result.append(QLatin1Char('%'));
            ++i;
            continue;
        }
        const QChar token = template_.at(i + 1);
        if (token == QLatin1Char('%'))
        {
            result.append(QLatin1Char('%'));
            i += 2;
            continue;
        }
        QString value;
        switch (token.toLatin1())
        {
        case 'f':
            value = filePath;
            break;
        case 'n':
            value = baseName;
            break;
        case 'd':
            value = dir;
            break;
        case 'e':
            value = ext;
            break;
        case 'p':
            value = projectDir;
            break;
        case 's':
            value = selectedText;
            break;
        case 'l':
            value = QString::number(lineNumber);
            break;
        default:
            result.append(QLatin1Char('%'));
            ++i;
            continue;
        }
        result.append(value);
        i += 2;
    }
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
    bool tokenStarted = false; // an empty quoted token ("") is still a token

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
#ifndef Q_OS_WIN
            else if (c == QLatin1Char('\\') && i + 1 < args.size())
            {
                current += args[++i];
            }
#endif
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
                tokenStarted = true;
            }
            else if (c == QLatin1Char('"'))
            {
                inDoubleQuote = true;
                tokenStarted = true;
            }
#ifndef Q_OS_WIN
            else if (c == QLatin1Char('\\') && i + 1 < args.size())
            {
                current += args[++i];
            }
#endif
            else if (c.isSpace())
            {
                if (tokenStarted)
                {
                    result << current;
                    current.clear();
                    tokenStarted = false;
                }
            }
            else
            {
                current += c;
                tokenStarted = true;
            }
        }
    }

    if (tokenStarted)
        result << current;

    return result;
}

QString ExternalToolManager::shellEscape(const QString& s)
{
    QString escaped = s;
    escaped.replace(QLatin1Char('\''), QStringLiteral("'\\''"));
    return QLatin1Char('\'') + escaped + QLatin1Char('\'');
}

#ifdef Q_OS_WIN
QString ExternalToolManager::cmdQuote(const QString& s)
{
    QString quoted = s;
    quoted.replace(QLatin1Char('"'), QStringLiteral("\"\""));
    return QLatin1Char('"') + quoted + QLatin1Char('"');
}
#endif

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
        // Terminal path: build a properly escaped command line for the platform shell.
        QString cmdLine;
#ifdef Q_OS_WIN
        // cmd.exe has no `exec` builtin, needs `cd /d` for cross-drive changes,
        // and quotes with double quotes only.
        cmdLine = QStringLiteral("cd /d %1 && %2").arg(cmdQuote(resolvedWorkDir), cmdQuote(resolvedCmd));
        for (const auto& arg : resolvedArgs)
        {
            cmdLine += QLatin1Char(' ') + cmdQuote(arg);
        }
#else
        // POSIX shell: single-quote escaping, `cd` + `exec`.
        cmdLine = QStringLiteral("cd %1 && exec %2").arg(shellEscape(resolvedWorkDir), shellEscape(resolvedCmd));
        for (const auto& arg : resolvedArgs)
        {
            cmdLine += QLatin1Char(' ') + shellEscape(arg);
        }
#endif
        cmdLine += QLatin1Char('\n');
        terminalSender(cmdLine);
    }
    else
    {
        // Background QProcess path with output dialog
        auto* outputDialog = new QDialog(parent);
        outputDialog->setWindowTitle(tool.name + ExternalToolManager::tr(" - Output"));
        outputDialog->setMinimumSize(500, 300);
        auto* layout = new QVBoxLayout(outputDialog);

        auto* process = new QProcess(outputDialog);
        process->setWorkingDirectory(resolvedWorkDir);
        process->setProcessChannelMode(QProcess::MergedChannels);

        auto* outputText = new QTextEdit(outputDialog);
        outputText->setReadOnly(true);
        outputText->setFont(QFontDatabase::systemFont(QFontDatabase::FixedFont));
        layout->addWidget(outputText);

        auto* closeBtn = new QPushButton(ExternalToolManager::tr("Close"), outputDialog);
        layout->addWidget(closeBtn);

        QPointer<QTextEdit> textGuard(outputText);
        QObject::connect(closeBtn, &QPushButton::clicked, outputDialog, &QDialog::accept);
        QObject::connect(process, &QProcess::readyReadStandardOutput, outputDialog,
                         [process, textGuard]()
                         {
                             if (textGuard)
                                 textGuard->append(QString::fromLocal8Bit(process->readAllStandardOutput()));
                         });
        QObject::connect(process, &QProcess::errorOccurred, outputDialog,
                         [textGuard](QProcess::ProcessError error)
                         {
                             if (!textGuard)
                                 return;
                             QString reason;
                             switch (error)
                             {
                             case QProcess::FailedToStart:
                                 reason = ExternalToolManager::tr("Failed to start (command not found or not executable)");
                                 break;
                             case QProcess::Crashed:
                                 reason = ExternalToolManager::tr("Process crashed");
                                 break;
                             case QProcess::Timedout:
                                 reason = ExternalToolManager::tr("Process timed out");
                                 break;
                             case QProcess::WriteError:
                                 reason = ExternalToolManager::tr("Write error");
                                 break;
                             case QProcess::ReadError:
                                 reason = ExternalToolManager::tr("Read error");
                                 break;
                             default:
                                 reason = ExternalToolManager::tr("Unknown error");
                                 break;
                             }
                             textGuard->append(ExternalToolManager::tr("\n--- Error: %1 ---").arg(reason));
                         });
        QObject::connect(process, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished), outputDialog,
                         [textGuard, process](int, QProcess::ExitStatus)
                         {
                             if (!textGuard)
                                 return;
                             textGuard->append(ExternalToolManager::tr("\n--- Process exited ---"));
                             textGuard->append(QString::fromLocal8Bit(process->readAllStandardOutput()));
                         });
        QObject::connect(outputDialog, &QDialog::finished, process, &QProcess::kill);
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
