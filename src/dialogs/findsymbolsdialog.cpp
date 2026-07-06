#include "findsymbolsdialog.h"
#include "lsp/lsptypes.h"
#include "lsp/lspservermanager.h"

#include <QLineEdit>
#include <QTreeView>
#include <QStandardItemModel>
#include <QLabel>
#include <QTimer>
#include <QPushButton>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonObject>
#include <QKeyEvent>

FindSymbolsDialog::FindSymbolsDialog(lsp::LspServerManager* lspManager, QWidget* parent)
    : QDialog(parent)
    , m_lspManager(lspManager)
{
    setWindowTitle(tr("Find Symbol"));
    setMinimumSize(400, 300);
    resize(500, 400);
    setupUI();

    m_debounceTimer = new QTimer(this);
    m_debounceTimer->setSingleShot(true);
    m_debounceTimer->setInterval(300);
    connect(m_debounceTimer, &QTimer::timeout, this, &FindSymbolsDialog::performSearch);

    connect(m_lspManager, &lsp::LspServerManager::workspaceSymbolsReady,
            this, &FindSymbolsDialog::onSymbolsReady);
}

FindSymbolsDialog::~FindSymbolsDialog() = default;

void FindSymbolsDialog::setupUI()
{
    auto* layout = new QVBoxLayout(this);

    auto* searchLayout = new QHBoxLayout();
    m_searchEdit = new QLineEdit(this);
    m_searchEdit->setPlaceholderText(tr("Type to search symbols..."));
    m_searchEdit->setClearButtonEnabled(true);
    searchLayout->addWidget(m_searchEdit);
    layout->addLayout(searchLayout);

    m_model = new QStandardItemModel(0, 3, this);
    m_model->setHorizontalHeaderLabels({tr("Name"), tr("Kind"), tr("File")});

    m_treeView = new QTreeView(this);
    m_treeView->setModel(m_model);
    m_treeView->setRootIsDecorated(false);
    m_treeView->setAlternatingRowColors(true);
    m_treeView->setSelectionMode(QAbstractItemView::SingleSelection);
    m_treeView->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_treeView->header()->setStretchLastSection(true);
    m_treeView->header()->setSectionResizeMode(0, QHeaderView::Stretch);
    m_treeView->header()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
    m_treeView->header()->setSectionResizeMode(2, QHeaderView::ResizeToContents);
    m_treeView->setSortingEnabled(true);
    layout->addWidget(m_treeView);

    m_statusLabel = new QLabel(tr("Type to search"), this);
    layout->addWidget(m_statusLabel);

    auto* buttonLayout = new QHBoxLayout();
    m_closeButton = new QPushButton(tr("Close"), this);
    buttonLayout->addStretch();
    buttonLayout->addWidget(m_closeButton);
    layout->addLayout(buttonLayout);

    connect(m_searchEdit, &QLineEdit::textChanged, this, &FindSymbolsDialog::onSearchTextChanged);
    connect(m_treeView, &QTreeView::doubleClicked, this, &FindSymbolsDialog::onItemDoubleClicked);
    connect(m_treeView, &QTreeView::activated, this, &FindSymbolsDialog::onItemActivated);
    connect(m_closeButton, &QPushButton::clicked, this, &QDialog::close);
}

void FindSymbolsDialog::onSearchTextChanged(const QString& text)
{
    if (text.isEmpty()) {
        m_model->removeRows(0, m_model->rowCount());
        m_statusLabel->setText(tr("Type to search"));
        m_debounceTimer->stop();
        return;
    }
    m_debounceTimer->start();
}

void FindSymbolsDialog::performSearch()
{
    QString query = m_searchEdit->text().trimmed();
    if (query.isEmpty()) return;

    m_statusLabel->setText(tr("Searching..."));
    m_model->removeRows(0, m_model->rowCount());

    // Search across all languages
    for (const QString& lang : m_lspManager->languages()) {
        auto* client = m_lspManager->clientForLanguage(lang);
        if (client)
            client->requestWorkspaceSymbols(query);
    }
}

void FindSymbolsDialog::onSymbolsReady(const QString& query, const QJsonArray& symbols)
{
    if (query != m_searchEdit->text().trimmed()) return;
    populateResults(symbols);
}

void FindSymbolsDialog::populateResults(const QJsonArray& symbols)
{
    for (const auto& s : symbols) {
        QJsonObject obj = s.toObject();
        QString name = obj["name"].toString();
        QString kind = obj["kind"].toString();

        QString location;
        QString filePath;
        int line = 0, col = 0;
        if (obj.contains("location")) {
            auto loc = obj["location"].toObject();
            filePath = lsp::pathFromUri(loc["uri"].toString());
            location = QFileInfo(filePath).fileName();
            auto range = loc["range"].toObject();
            line = range["start"].toObject()["line"].toInt();
            col = range["start"].toObject()["character"].toInt();
        }

        QString icon;
        if (kind == "Function" || kind == "Method") icon = QStringLiteral("\u0192");
        else if (kind == "Class" || kind == "Struct") icon = QStringLiteral("\u2299");
        else if (kind == "Variable" || kind == "Field") icon = QStringLiteral("\u25CB");
        else if (kind == "Interface") icon = QStringLiteral("\u2261");
        else if (kind == "Module" || kind == "Namespace") icon = QStringLiteral("\u25A0");
        else if (kind == "Enum") icon = QStringLiteral("\u2630");
        else icon = QStringLiteral("\u2022");

        auto* nameItem = new QStandardItem(icon + QStringLiteral(" ") + name);
        nameItem->setEditable(false);
        nameItem->setData(filePath, Qt::UserRole + 1);
        nameItem->setData(line, Qt::UserRole + 2);
        nameItem->setData(col, Qt::UserRole + 3);

        auto* kindItem = new QStandardItem(kind);
        kindItem->setEditable(false);

        auto* fileItem = new QStandardItem(location);
        fileItem->setEditable(false);

        m_model->appendRow({nameItem, kindItem, fileItem});
    }

    m_statusLabel->setText(tr("%1 symbols found").arg(m_model->rowCount()));
}

void FindSymbolsDialog::onItemDoubleClicked(const QModelIndex& index)
{
    onItemActivated(index);
}

void FindSymbolsDialog::onItemActivated(const QModelIndex& index)
{
    if (!index.isValid()) return;
    QStandardItem* item = m_model->item(index.row(), 0);
    if (!item) return;

    QString filePath = item->data(Qt::UserRole + 1).toString();
    int line = item->data(Qt::UserRole + 2).toInt();
    int col = item->data(Qt::UserRole + 3).toInt();

    if (!filePath.isEmpty())
        emit navigateToSymbol(filePath, line, col);
}
