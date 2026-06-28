#ifndef ERRORLISTPANEL_H
#define ERRORLISTPANEL_H

#include <QDockWidget>
#include <QTreeView>
#include <QStandardItemModel>
#include <QComboBox>
#include <QLabel>
#include "lsp/lsptypes.h"

class ErrorListPanel : public QDockWidget
{
    Q_OBJECT

public:
    explicit ErrorListPanel(QWidget* parent = nullptr);
    ~ErrorListPanel() override;

    void updateDiagnostics(const QString& uri, const QList<lsp::Diagnostic>& diagnostics);
    void clearDiagnostics(const QString& uri);
    void clearAll();

signals:
    void navigateToLocation(const QString& filePath, int line, int column);

private slots:
    void onItemDoubleClicked(const QModelIndex& index);
    void onFilterChanged(int index);

private:
    void setupUI();

    QTreeView* m_treeView;
    QStandardItemModel* m_model;
    QComboBox* m_filterCombo;
    QLabel* m_summaryLabel;

    QHash<QString, QList<lsp::Diagnostic>> m_allDiagnostics;
};

#endif // ERRORLISTPANEL_H
