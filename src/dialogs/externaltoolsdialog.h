#ifndef EXTERNALTOOLSDIALOG_H
#define EXTERNALTOOLSDIALOG_H

#include "dialogsettings.h"

#include <QDialog>

class QTableWidget;
class QPushButton;
class QCloseEvent;

class ExternalToolsDialog : public QDialog
{
    Q_OBJECT

public:
    explicit ExternalToolsDialog(QWidget* parent = nullptr);
    ~ExternalToolsDialog() override;

protected:
    void closeEvent(QCloseEvent* event) override;

private:
    void setupUI();
    void loadTools();
    void saveTools();

    QTableWidget* m_table = nullptr;
    QPushButton* m_addButton = nullptr;
    QPushButton* m_editButton = nullptr;
    QPushButton* m_removeButton = nullptr;
    DialogSettings m_geometrySettings;
};

#endif
