#ifndef EXTERNALTOOLSDIALOG_H
#define EXTERNALTOOLSDIALOG_H

#include <QDialog>
#include "dialogsettings.h"

class QTableWidget;
class QPushButton;
class QCloseEvent;

class ExternalToolsDialog : public QDialog {
    Q_OBJECT

public:
    explicit ExternalToolsDialog(QWidget *parent = nullptr);
    ~ExternalToolsDialog() override;

protected:
    void closeEvent(QCloseEvent *event) override;

private:
    void setupUI();
    void loadTools();
    void saveTools();

    QTableWidget *m_table;
    QPushButton *m_addButton;
    QPushButton *m_editButton;
    QPushButton *m_removeButton;
    DialogSettings m_geometrySettings;
};

#endif
