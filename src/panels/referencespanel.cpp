#include "referencespanel.h"

#include "lsp/lsptypes.h"

#include <QFileInfo>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QVBoxLayout>

ReferencesPanel::ReferencesPanel(QWidget* parent) : QDockWidget(tr("References"), parent)
{
    setObjectName("ReferencesPanel");
    setFeatures(QDockWidget::DockWidgetClosable | QDockWidget::DockWidgetFloatable | QDockWidget::DockWidgetMovable);
    setupUI();
}

ReferencesPanel::~ReferencesPanel() = default;

void ReferencesPanel::setupUI()
{
    setTitleBarWidget(new QWidget(this));
    QWidget* content = new QWidget(this);
    QVBoxLayout* layout = new QVBoxLayout(content);
    layout->setContentsMargins(4, 4, 4, 4);
    layout->setSpacing(4);

    m_summaryLabel = new QLabel(tr("No references"), content);
    layout->addWidget(m_summaryLabel);

    m_model = new QStandardItemModel(0, 3, content);
    m_model->setHorizontalHeaderLabels({tr("File"), tr("Line"), tr("Code")});

    m_treeView = new QTreeView(content);
    m_treeView->setModel(m_model);
    m_treeView->setRootIsDecorated(false);
    m_treeView->setAlternatingRowColors(true);
    m_treeView->setSelectionMode(QAbstractItemView::SingleSelection);
    m_treeView->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_treeView->header()->setStretchLastSection(true);
    m_treeView->header()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
    m_treeView->header()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
    m_treeView->header()->setSectionResizeMode(2, QHeaderView::Stretch);
    m_treeView->setSortingEnabled(true);

    layout->addWidget(m_treeView);
    setWidget(content);

    connect(m_treeView, &QTreeView::doubleClicked, this, &ReferencesPanel::onItemDoubleClicked);
}

void ReferencesPanel::setReferences(const QList<lsp::Location>& locations)
{
    m_model->removeRows(0, m_model->rowCount());

    // Group locations by file so the tree reads like a Find-in-Files result list.
    QHash<QString, QList<lsp::Location>> byFile;
    for (const auto& loc : locations)
        byFile[loc.uri].append(loc);

    int total = 0;
    for (auto it = byFile.begin(); it != byFile.end(); ++it)
    {
        const QString& uri = it.key();
        const QString filePath = lsp::pathFromUri(uri);
        const QString fileName = QFileInfo(filePath).fileName();

        for (const auto& loc : it.value())
        {
            total++;

            const int line = loc.range.start.line;
            const int col = loc.range.start.character;
            QString startLineText;
            // We only have the URI here, not the document text, so show the
            // location rather than a snippet of code.
            QString code = QString("%1:%2").arg(line + 1).arg(col + 1);

            auto* fileItem = new QStandardItem(fileName);
            fileItem->setEditable(false);
            fileItem->setToolTip(filePath);
            fileItem->setData(filePath, FILE_PATH_ROLE);

            auto* lineItem = new QStandardItem(QString::number(line + 1));
            lineItem->setEditable(false);

            auto* codeItem = new QStandardItem(code);
            codeItem->setEditable(false);
            codeItem->setData(line, LINE_ROLE);
            codeItem->setData(col, COLUMN_ROLE);

            m_model->appendRow({fileItem, lineItem, codeItem});
        }
    }

    m_summaryLabel->setText(total == 0 ? tr("No references found")
                                       : tr("%1 reference%2 in %3 file%4")
                                           .arg(total)
                                           .arg(total == 1 ? QString() : QStringLiteral("s"))
                                           .arg(byFile.size())
                                           .arg(byFile.size() == 1 ? QString() : QStringLiteral("s")));
}

void ReferencesPanel::clear()
{
    m_model->removeRows(0, m_model->rowCount());
    m_summaryLabel->setText(tr("No references"));
}

void ReferencesPanel::onItemDoubleClicked(const QModelIndex& index)
{
    if (!index.isValid())
        return;

    QStandardItem* fileItem = m_model->item(index.row(), 0);
    QStandardItem* codeItem = m_model->item(index.row(), 2);
    if (!fileItem || !codeItem)
        return;

    QString filePath = fileItem->data(FILE_PATH_ROLE).toString();
    int line = codeItem->data(LINE_ROLE).toInt();
    int column = codeItem->data(COLUMN_ROLE).toInt();

    if (!filePath.isEmpty())
        emit navigateToLocation(filePath, line, column);
}
