#include "externaltoolmanager.h"

#include <QFormLayout>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QDialogButtonBox>
#include <QProcess>
#include <QDir>
#include <QFileInfo>
#include <QTextEdit>

ExternalToolManager* ExternalToolManager::s_instance = nullptr;

ExternalToolManager& ExternalToolManager::instance() {
    if (!s_instance) {
        s_instance = new ExternalToolManager();
    }
    return *s_instance;
}

ExternalToolManager::ExternalToolManager(QObject *parent) : QObject(parent) {
    if (!s_instance) {
        s_instance = this;
    }
}

QList<ExternalTool> ExternalToolManager::tools() const {
    return m_tools;
}

void ExternalToolManager::setTools(const QList<ExternalTool>& tools) {
    m_tools = tools;
    emit toolsChanged();
}

QString ExternalToolManager::resolveVariables(const QString& template_, const QString& filePath,
                                               const QString& projectDir, const QString& selectedText,
                                               int lineNumber) const {
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

QString ExternalToolManager::workingDirForTool(const ExternalTool& tool, const QString& filePath,
                                                const QString& projectDir) const {
    QString wd = resolveVariables(tool.workingDir, filePath, projectDir, QString(), 0);
    if (wd.isEmpty()) {
        if (!projectDir.isEmpty())
            wd = projectDir;
        else if (!filePath.isEmpty())
            wd = QFileInfo(filePath).absolutePath();
        else
            wd = QDir::homePath();
    }
    return wd;
}

ExternalToolEditDialog::ExternalToolEditDialog(QWidget *parent)
    : QDialog(parent) {
    setupUI();
}

ExternalToolEditDialog::ExternalToolEditDialog(const ExternalTool& tool, QWidget *parent)
    : QDialog(parent) {
    setupUI();
    setTool(tool);
}

ExternalTool ExternalToolEditDialog::tool() const {
    ExternalTool t;
    t.name = m_nameEdit->text().trimmed();
    t.command = m_commandEdit->text().trimmed();
    t.arguments = m_argumentsEdit->text().trimmed();
    t.workingDir = m_workDirEdit->text().trimmed();
    t.shortcut = m_shortcutEdit->keySequence();
    t.runInTerminal = m_runInTerminalCheck->isChecked();
    return t;
}

void ExternalToolEditDialog::setTool(const ExternalTool& tool) {
    m_nameEdit->setText(tool.name);
    m_commandEdit->setText(tool.command);
    m_argumentsEdit->setText(tool.arguments);
    m_workDirEdit->setText(tool.workingDir);
    m_shortcutEdit->setKeySequence(tool.shortcut);
    m_runInTerminalCheck->setChecked(tool.runInTerminal);
}

void ExternalToolEditDialog::setupUI() {
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

    m_hintLabel = new QLabel(
        tr("Variables: %f = full path, %n = filename, %d = directory, "
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
