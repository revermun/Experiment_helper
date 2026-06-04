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
#include <QFrame>
#include <QVBoxLayout>

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
    void saveAboutExperiment(QString dir = "");
    void saveConnections(QString dir = "");
    void saveMounting(QString dir = "");
    static QMap<QString, ExperimentConnectionInfo> getConnectionsFromFile(QString dir);
    static QMap<QString, ExperimentDeviceInfo> getDevicesFromFile(QString dir);
    static QMap<QString, ExperimentGroupBoxParametersInfo> getMountingFromFile(QString dir);

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
    void changeGroupBoxInfo();
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

    QFrame* frameIMU;
    QLabel *labelInstabilityBias;
    QDoubleSpinBox *spinInstabilityBias;
    QLabel *labelRandomWalk;
    QDoubleSpinBox *spinRandomWalk;
    QLabel *labelInitialError;
    QDoubleSpinBox *spinInitialError;

    QFrame* frameCamera;
    QLabel *labelFPS;
    QSpinBox *spinFPS;
    QLabel *labelHeight;
    QSpinBox *spinHeight;
    QLabel *labelWidth;
    QSpinBox *spinWidth;

    QFrame* frameAntenna;
    QLabel *labelDirectory;
    QLineEdit *lineDirectory;

    QDialog     *deviceDialog;
    QLabel      *deviceLabelID;
    QLineEdit   *deviceLineID;
    QVBoxLayout *deviceLayout;
    QHBoxLayout *deviceLayoutLines;
    QHBoxLayout *deviceLayoutGroupBoxes;
    QHBoxLayout *deviceHLayout;
    QLabel      *deviceLabelModel;
    QLineEdit   *deviceLineModel;
    QLabel      *deviceLabelType;
    QComboBox   *deviceComboType;
    QGroupBox   *deviceGroupOutputs;
    QGroupBox   *deviceGroupInfo;
    QListWidget *deviceListOutputs;
    QPushButton *deviceAddButton;
    QPushButton *deviceDeleteButton;
    QPushButton *deviceOkButton;
    QPushButton *deviceCancelButton;
    QVBoxLayout *deviceGroupOutputsLayout;
    QVBoxLayout *deviceGroupInfoLayout;
    QVBoxLayout *deviceLayoutOutputButtons;

    QGroupBox* createParameterGroup(QString parameter, QWidget* parent = nullptr);
    QGroupBox* createParametersGroup(QString id, bool isCheckType = true);
    void setupTableSize(QTableWidget* table);
    void addSaveButton(int index);
    void openTeamMemberDialog(QString FIO = "", QString role = "организатор");
    void sortListWidgetByDeviceType(QListWidget* listWidget, Qt::SortOrder order);
    QGroupBox* addParameter(QString parameter,  QGroupBox* group);
    QList<QGroupBox*> getParameters(QGroupBox* group);
    void loadAboutExperiment();
    void loadConnections();
    void loadMounting();
    void setupWidgets();
};

#endif // EXPERIMENTCONFIGURATIONDIALOG_H
