#include "errorlistpanel.h"
#include "lsp/lsptypes.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QFileInfo>

enum {
    FILE_PATH_ROLE = Qt::UserRole + 100,
    LINE_ROLE = Qt::UserRole + 101,
    COLUMN_ROLE = Qt::UserRole + 102
};

ErrorListPanel::ErrorListPanel(QWidget* parent)
    : QDockWidget(tr("Error List"), parent)
{
    setObjectName("ErrorListPanel");
    setupUI();
}

ErrorListPanel::~ErrorListPanel() = default;

void ErrorListPanel::setupUI()
{
    QWidget* content = new QWidget(this);
    QVBoxLayout* layout = new QVBoxLayout(content);
    layout->setContentsMargins(4, 4, 4, 4);
    layout->setSpacing(4);

    QHBoxLayout* filterLayout = new QHBoxLayout();
    m_filterCombo = new QComboBox(content);
    m_filterCombo->addItem(tr("All"));
    m_filterCombo->addItem(tr("Errors"));
    m_filterCombo->addItem(tr("Warnings"));
    m_filterCombo->addItem(tr("Info"));
    m_filterCombo->addItem(tr("Hints"));
    filterLayout->addWidget(new QLabel(tr("Show:"), content));
    filterLayout->addWidget(m_filterCombo);
    filterLayout->addStretch();

    m_summaryLabel = new QLabel(tr("No issues"), content);
    filterLayout->addWidget(m_summaryLabel);
    layout->addLayout(filterLayout);

    m_model = new QStandardItemModel(0, 4, content);
    m_model->setHorizontalHeaderLabels({tr("Severity"), tr("Message"), tr("File"), tr("Line")});

    m_treeView = new QTreeView(content);
    m_treeView->setModel(m_model);
    m_treeView->setRootIsDecorated(false);
    m_treeView->setAlternatingRowColors(true);
    m_treeView->setSelectionMode(QAbstractItemView::SingleSelection);
    m_treeView->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_treeView->header()->setStretchLastSection(true);
    m_treeView->header()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
    m_treeView->header()->setSectionResizeMode(1, QHeaderView::Stretch);
    m_treeView->header()->setSectionResizeMode(2, QHeaderView::ResizeToContents);
    m_treeView->header()->setSectionResizeMode(3, QHeaderView::ResizeToContents);
    m_treeView->setSortingEnabled(true);

    layout->addWidget(m_treeView);
    setWidget(content);

    connect(m_treeView, &QTreeView::doubleClicked, this, &ErrorListPanel::onItemDoubleClicked);
    connect(m_filterCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &ErrorListPanel::onFilterChanged);
}

void ErrorListPanel::updateDiagnostics(const QString& uri, const QList<lsp::Diagnostic>& diagnostics)
{
    m_allDiagnostics[uri] = diagnostics;
    onFilterChanged(m_filterCombo->currentIndex());
}

void ErrorListPanel::clearDiagnostics(const QString& uri)
{
    m_allDiagnostics.remove(uri);
    onFilterChanged(m_filterCombo->currentIndex());
}

void ErrorListPanel::clearAll()
{
    m_allDiagnostics.clear();
    m_model->removeRows(0, m_model->rowCount());
    m_summaryLabel->setText(tr("No issues"));
}

void ErrorListPanel::onFilterChanged(int index)
{
    m_model->removeRows(0, m_model->rowCount());

    int totalCount = 0;
    int shownCount = 0;

    for (auto it = m_allDiagnostics.begin(); it != m_allDiagnostics.end(); ++it) {
        const QString& uri = it.key();
        const auto& diagnostics = it.value();
        QString filePath = lsp::pathFromUri(uri);
        QString fileName = QFileInfo(filePath).fileName();

        for (const auto& d : diagnostics) {
            totalCount++;
            bool show = false;
            switch (index) {
            case 0: show = true; break;
            case 1: show = (d.severityLevel == 1); break;
            case 2: show = (d.severityLevel == 2); break;
            case 3: show = (d.severityLevel == 3); break;
            case 4: show = (d.severityLevel == 4); break;
            }
            if (!show) continue;

            shownCount++;

            QList<QStandardItem*> row;

            QString icon;
            switch (d.severityLevel) {
            case 1: icon = QStringLiteral("\u2716"); break; // ✖ error
            case 2: icon = QStringLiteral("\u26A0"); break; // ⚠ warning
            case 3: icon = QStringLiteral("\u2139"); break; // ℹ info
            case 4: icon = QStringLiteral("\u24D8"); break; // ⓘ hint
            default: icon = QString();
            }

            auto* sevItem = new QStandardItem(icon + QStringLiteral(" ") + d.severity);
            sevItem->setEditable(false);
            sevItem->setData(d.severityLevel, Qt::UserRole);

            auto* msgItem = new QStandardItem(d.message);
            msgItem->setEditable(false);
            msgItem->setToolTip(d.message);

            auto* fileItem = new QStandardItem(fileName);
            fileItem->setEditable(false);
            fileItem->setToolTip(filePath);
            fileItem->setData(filePath, Qt::UserRole);

            auto* lineItem = new QStandardItem(QString::number(d.range.start.line + 1));
            lineItem->setEditable(false);

            row << sevItem << msgItem << fileItem << lineItem;
            m_model->appendRow(row);

            // Store location data in the first column for double-click navigation
            sevItem->setData(filePath, FILE_PATH_ROLE);
            sevItem->setData(d.range.start.line, LINE_ROLE);
            sevItem->setData(d.range.start.character, COLUMN_ROLE);
        }
    }

    m_summaryLabel->setText(tr("%1 / %2 issues shown").arg(shownCount).arg(totalCount));
}

void ErrorListPanel::onItemDoubleClicked(const QModelIndex& index)
{
    if (!index.isValid())
        return;

    QStandardItem* item = m_model->itemFromIndex(index);
    if (!item)
        return;

    // Navigate to the parent row's first column to get stored data
    QStandardItem* sevItem = m_model->item(index.row(), 0);
    if (!sevItem)
        return;

    QString filePath = sevItem->data(FILE_PATH_ROLE).toString();
    int line = sevItem->data(LINE_ROLE).toInt();
    int column = sevItem->data(COLUMN_ROLE).toInt();

    if (!filePath.isEmpty())
        emit navigateToLocation(filePath, line, column);
}
