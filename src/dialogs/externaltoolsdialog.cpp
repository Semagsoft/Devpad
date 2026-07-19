#include "externaltoolsdialog.h"

#include "externaltoolmanager.h"
#include "settingsmanager.h"

#include <QCloseEvent>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QMessageBox>
#include <QPushButton>
#include <QTableWidget>
#include <QVBoxLayout>

ExternalToolsDialog::ExternalToolsDialog(QWidget* parent) : QDialog(parent), m_geometrySettings("ExternalTools")
{
    setWindowTitle(tr("External Tools"));
    setModal(true);
    setMinimumSize(550, 350);
    setupUI();
    loadTools();
    m_geometrySettings.restoreGeometry(this);
}

ExternalToolsDialog::~ExternalToolsDialog() = default;

void ExternalToolsDialog::setupUI()
{
    auto* mainLayout = new QVBoxLayout(this);

    m_table = new QTableWidget(0, 5, this);
    m_table->setHorizontalHeaderLabels({tr("Name"), tr("Command"), tr("Arguments"), tr("Shortcut"), tr("Terminal")});
    m_table->horizontalHeader()->setStretchLastSection(false);
    m_table->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Stretch);
    m_table->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Stretch);
    m_table->horizontalHeader()->setSectionResizeMode(2, QHeaderView::Stretch);
    m_table->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_table->setSelectionMode(QAbstractItemView::SingleSelection);
    m_table->verticalHeader()->setVisible(false);
    m_table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    mainLayout->addWidget(m_table);

    auto* btnLayout = new QHBoxLayout();
    m_addButton = new QPushButton(tr("Add..."), this);
    m_editButton = new QPushButton(tr("Edit..."), this);
    m_removeButton = new QPushButton(tr("Remove"), this);
    btnLayout->addWidget(m_addButton);
    btnLayout->addWidget(m_editButton);
    btnLayout->addWidget(m_removeButton);

    auto* closeButton = new QPushButton(tr("Close"), this);
    btnLayout->addStretch();
    btnLayout->addWidget(closeButton);
    mainLayout->addLayout(btnLayout);

    auto setRowFromTool = [this](int row, const ExternalTool& tool)
    {
        auto* nameItem = new QTableWidgetItem(tool.name);
        nameItem->setData(Qt::UserRole, tool.workingDir);
        m_table->setItem(row, 0, nameItem);
        m_table->setItem(row, 1, new QTableWidgetItem(tool.command));
        m_table->setItem(row, 2, new QTableWidgetItem(tool.arguments));
        m_table->setItem(row, 3, new QTableWidgetItem(tool.shortcut.toString()));
        m_table->setItem(row, 4, new QTableWidgetItem(tool.runInTerminal ? tr("Yes") : tr("No")));
    };

    auto getToolFromRow = [this](int row) -> ExternalTool
    {
        ExternalTool tool;
        tool.name = m_table->item(row, 0)->text();
        tool.workingDir = m_table->item(row, 0)->data(Qt::UserRole).toString();
        tool.command = m_table->item(row, 1)->text();
        tool.arguments = m_table->item(row, 2)->text();
        tool.shortcut = QKeySequence(m_table->item(row, 3)->text());
        tool.runInTerminal = (m_table->item(row, 4)->text() == tr("Yes"));
        return tool;
    };

    connect(m_addButton, &QPushButton::clicked, this,
            [this, setRowFromTool]()
            {
                ExternalToolEditDialog dlg(this);
                if (dlg.exec() == QDialog::Accepted)
                {
                    ExternalTool tool = dlg.tool();
                    if (tool.name.isEmpty() || tool.command.isEmpty())
                        return;
                    int row = m_table->rowCount();
                    m_table->insertRow(row);
                    setRowFromTool(row, tool);
                }
            });

    connect(m_editButton, &QPushButton::clicked, this,
            [this, getToolFromRow, setRowFromTool]()
            {
                int row = m_table->currentRow();
                if (row < 0)
                    return;
                ExternalToolEditDialog dlg(getToolFromRow(row), this);
                if (dlg.exec() == QDialog::Accepted)
                {
                    ExternalTool tool = dlg.tool();
                    if (tool.name.isEmpty() || tool.command.isEmpty())
                        return;
                    setRowFromTool(row, tool);
                }
            });

    connect(m_removeButton, &QPushButton::clicked, this,
            [this]()
            {
                int row = m_table->currentRow();
                if (row >= 0)
                    m_table->removeRow(row);
            });

    connect(m_table, &QTableWidget::itemSelectionChanged, this,
            [this]()
            {
                bool hasSelection = m_table->currentRow() >= 0;
                m_editButton->setEnabled(hasSelection);
                m_removeButton->setEnabled(hasSelection);
            });
    m_editButton->setEnabled(false);
    m_removeButton->setEnabled(false);

    connect(closeButton, &QPushButton::clicked, this,
            [this]()
            {
                saveTools();
                accept();
            });
}

void ExternalToolsDialog::loadTools()
{
    auto& s = SettingsManager::instance();
    int toolCount = s.externalToolCount();
    for (int i = 0; i < toolCount; ++i)
    {
        ExternalTool tool;
        tool.name = s.externalToolName(i);
        tool.command = s.externalToolCommand(i);
        tool.arguments = s.externalToolArguments(i);
        tool.workingDir = s.externalToolWorkingDir(i);
        tool.shortcut = QKeySequence(s.externalToolShortcut(i));
        tool.runInTerminal = s.externalToolRunInTerminal(i);
        int row = m_table->rowCount();
        m_table->insertRow(row);
        auto* nameItem = new QTableWidgetItem(tool.name);
        nameItem->setData(Qt::UserRole, tool.workingDir);
        m_table->setItem(row, 0, nameItem);
        m_table->setItem(row, 1, new QTableWidgetItem(tool.command));
        m_table->setItem(row, 2, new QTableWidgetItem(tool.arguments));
        m_table->setItem(row, 3, new QTableWidgetItem(tool.shortcut.toString()));
        m_table->setItem(row, 4, new QTableWidgetItem(tool.runInTerminal ? tr("Yes") : tr("No")));
    }
}

void ExternalToolsDialog::closeEvent(QCloseEvent* event)
{
    saveTools();
    m_geometrySettings.saveGeometry(this);
    QDialog::closeEvent(event);
}

void ExternalToolsDialog::saveTools()
{
    auto& s = SettingsManager::instance();
    int oldCount = s.externalToolCount();
    for (int i = oldCount - 1; i >= 0; --i)
        s.removeExternalTool(i);

    int newCount = m_table->rowCount();
    for (int i = 0; i < newCount; ++i)
    {
        QString workDir = m_table->item(i, 0)->data(Qt::UserRole).toString();
        s.addExternalTool(m_table->item(i, 0)->text(), m_table->item(i, 1)->text(), m_table->item(i, 2)->text(), workDir, m_table->item(i, 3)->text(),
                          m_table->item(i, 4)->text() == tr("Yes"));
    }
}
