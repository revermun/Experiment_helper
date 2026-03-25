#ifndef EXPERIMENTCONFIGURATIONDIALOG_H
#define EXPERIMENTCONFIGURATIONDIALOG_H

#include <QDialog>
#include <QDebug>
#include <QGroupBox>
#include <QListWidget>
#include <QMessageBox>

#include "enums.h"

namespace Ui {
class experimentConfigurationDialog;
}

class experimentConfigurationDialog : public QDialog
{
    Q_OBJECT

public:
    explicit experimentConfigurationDialog(QWidget *parent = nullptr);
    ~experimentConfigurationDialog();

public slots:
    void addTeamMember();
    void editTeamMember();
    void deleteTeamMember();
    void changeGeneralInfo(QString experimentType);
    void saveTab();
    void changeSaveButtons(int index);
    void addDevice();
    void editDevice();
    void sortDevices();
    void deleteDevice();
    void addConnection();
    void editConnection();
    void sortConnections();
    void deleteConnection();
    void changeGroupBoxInfo(QGroupBox *groupInfo);
    void changeGroupBoxSettings(QGroupBox* groupSettings);
    void addOutput(QListWidget *listOutputs);
    void deleteOutput(QListWidget *listOutputs);


private:
    void addSaveButton(int index);
    Ui::experimentConfigurationDialog *ui;
    void openTeamMemberDialog(QString FIO = "", QString role = "организатор");
    Qt::SortOrder devicesSort = Qt::AscendingOrder;
};

#endif // EXPERIMENTCONFIGURATIONDIALOG_H
