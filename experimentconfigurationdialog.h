#ifndef EXPERIMENTCONFIGURATIONDIALOG_H
#define EXPERIMENTCONFIGURATIONDIALOG_H

#include <QDialog>
#include <QDebug>
#include <QGroupBox>
#include <QListWidget>
#include <QMessageBox>
#include <QComboBox>
#include <QDoubleSpinBox>
#include <QSpinBox>
#include <QTableWidget>
#include <yaml-cpp/yaml.h>
#include <fstream>
#include <QFile>

#include "enums.h"
#include "structs.h"

namespace Ui {
class experimentConfigurationDialog;
}

class experimentConfigurationDialog : public QDialog
{
    Q_OBJECT

public:
    explicit experimentConfigurationDialog(QString experimentDirectory,QWidget *parent = nullptr);
    ~experimentConfigurationDialog();

    void saveAll();
    void changeDir(QString dir);

public slots:
    void saveTab();
    void addDevice();
    void editDevice();
    void sortDevices();
    void deleteDevice();
    void addParameter();
    void addTeamMember();
    void addConnection();
    void editConnection();
    void removeParameter();
    void deleteTeamMember();
    void deleteConnection();
    void addParametersEvent();
    void removePrametersGroup();
    void changeSaveButtons(int index);
    void addOutput(QListWidget *listOutputs);
    void deleteOutput(QListWidget *listOutputs);
    void changeGroupBoxInfo(QGroupBox *groupInfo);
    void changeGeneralInfo(QString experimentType);
    void changeGroupBoxSettings(QGroupBox* groupSettings);
    void changeDeviceOutputs(QComboBox* comboDeviceOutput, bool isEditing = false, QString device2 = "");
    void checkConnectionSettingsFields(QComboBox* comboDevice1, QComboBox* comboDevice2, QDialog* dialog);


private:
    QString currentDirectory;
    QString aboutExperimentDirectory;
    QString experimentConnectionsDirectory;
    QString experimentMountingDirectory;

    QMap<QString,ExperimentDeviceInfo> devicesMap;
    QMap<QString,ExperimentConnectionInfo> connectionsMap;
    QMap<QString,ExperimentGroupBoxParametersInfo> groupBoxParametersMap;

    Ui::experimentConfigurationDialog *ui;
    Qt::SortOrder devicesSort = Qt::AscendingOrder;

    QGroupBox* createParameterGroup(QString parameter, QWidget* parent = nullptr);
    QGroupBox* createParametersGroup(QString id, bool isCheckType = true);
    void setupTableSize(QTableWidget* table);
    void addSaveButton(int index);
    void openTeamMemberDialog(QString FIO = "", QString role = "организатор");
    void sortListWidgetByDeviceType(QListWidget* listWidget, Qt::SortOrder order);
    QGroupBox* addParameter(QString parameter,  QGroupBox* group);
    QList<QGroupBox*> getParameters(QGroupBox* group);
    void saveAboutExperiment();
    void saveConnections();
    void saveMounting();
};

#endif // EXPERIMENTCONFIGURATIONDIALOG_H
