#ifndef FINDSYMBOLSDIALOG_H
#define FINDSYMBOLSDIALOG_H

#include <QDialog>

class QLineEdit;
class QTreeView;
class QStandardItemModel;
class QLabel;
class QTimer;
class QPushButton;

namespace lsp {
class LspServerManager;
}

class FindSymbolsDialog : public QDialog
{
    Q_OBJECT

public:
    explicit FindSymbolsDialog(lsp::LspServerManager* lspManager, QWidget* parent = nullptr);
    ~FindSymbolsDialog() override;

signals:
    void navigateToSymbol(const QString& filePath, int line, int column);

private slots:
    void onSearchTextChanged(const QString& text);
    void performSearch();
    void onSymbolsReady(const QString& query, const QJsonArray& symbols);
    void onItemDoubleClicked(const QModelIndex& index);
    void onItemActivated(const QModelIndex& index);

private:
    void setupUI();
    void populateResults(const QJsonArray& symbols);

    lsp::LspServerManager* m_lspManager;
    QLineEdit* m_searchEdit;
    QTreeView* m_treeView;
    QStandardItemModel* m_model;
    QLabel* m_statusLabel;
    QTimer* m_debounceTimer;
    QPushButton* m_closeButton;
    int m_navigateLine = -1;
    int m_navigateColumn = -1;
};

#endif // FINDSYMBOLSDIALOG_H
