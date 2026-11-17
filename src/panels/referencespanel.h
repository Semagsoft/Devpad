#ifndef REFERENCESPANEL_H
#define REFERENCESPANEL_H

#include "lsp/lsptypes.h"

#include <QDockWidget>
#include <QLabel>
#include <QStandardItemModel>
#include <QTreeView>

class ReferencesPanel : public QDockWidget
{
    Q_OBJECT

public:
    explicit ReferencesPanel(QWidget* parent = nullptr);
    ~ReferencesPanel() override;

    // Replace the current reference list with the locations returned by a
    // textDocument/references request. Locations may span multiple files.
    void setReferences(const QList<lsp::Location>& locations);
    void clear();

signals:
    // line: 0-based. column: LSP UTF-16 code unit column.
    void navigateToLocation(const QString& filePath, int line, int column);

private slots:
    void onItemDoubleClicked(const QModelIndex& index);

private:
    void setupUI();

    QTreeView* m_treeView;
    QStandardItemModel* m_model;
    QLabel* m_summaryLabel = nullptr;

    static constexpr int FILE_PATH_ROLE = Qt::UserRole + 100;
    static constexpr int LINE_ROLE = Qt::UserRole + 101;
    static constexpr int COLUMN_ROLE = Qt::UserRole + 102;
};

#endif // REFERENCESPANEL_H
