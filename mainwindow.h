#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QMap>
#include <QList>
#include <QDomDocument>
#include <QFileDialog>
#include <QCheckBox>
#include <QSerialPort>
#include <QTableWidget>
#include <QTime>
#include <QTcpSocket>
#include <QScrollBar>
#include <QLabel>

#include "enums.h"
#include "structs.h"
#include "serialtotcpbridge.h"

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE


class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

    QMap<QString,NewData> getNewData();

private slots:
    void openConnectionSettings();
    void performAction(QAction* action);
    void deleteConnection();
    void editConnection();
    void openNotes();
    void openEventSettings();
    void openStartStopActions();
    void openDataAndGraphs();
    void connectDevice();
    void disconnectDevice();
    void startExperiment();
    void parseMessage();
    void sendUserEvent();
    void addConnectionFromFile();
    void clearLogTable();
    void readPorts();
    void indicateData();
    void onNewBridgeConnection();
    bool exploreExperiment();


signals:
    void newData(QString name);

private:
    void getMessagesConfig();
    void fillConnectionsTable();
    bool openPickDirectoryDialog();
    void addItemToConnectionsTable(DeviceInfo info);
    bool deleteDir(const QString &dirName, bool isDeleteOnlyContents = false);
    void setupTableSize(QTableWidget* table);
    void addItemToLogTable(QString localTime, QString GNSSTime, QString event);
    void loadConnections(QString connFileDir);
    void loadEvents(QString eventFileDir);
    bool loadExperiment(QString dir);
    void saveConnections(QString dir);
    void saveEvents(QString dir);
    void saveNotes(QString dir);
    void saveExperimentConfiguration(QString dir);
    void saveExperiment(QString dir);

    Ui::MainWindow *ui;

    //контейнеры
    QList<QList<QString>> notesList;
    QMap<QString,Mess> messagesMap;
    QMap<QString,EventData> eventMap;

    QMap<QString,DeviceInfo> devicesMap;
    QMap<QString,TableConnectionsFields> tableFieldsMap;
    QMap<QString, QObject*> connectionsMap;
    QMap<QString,QByteArray*> bufferMap;
    QMap<QString,NewData> newDataMap;
    QMap<QString,SerialToTcpBridge*> bridgeMap;

    //работа с файлами
    QDomDocument connectionsDoc;
    QDomElement connectionsRootElement;
    QDomDocument eventDoc;
    QDomElement eventRootElement;
    QString experimentDirectory;

    //флаги
    bool isLap = 0;
    int lapNumber = 0;
    bool eventSettingsSolFound = 0;
    bool eventSettingsSolLost = 0;
    bool eventSettingsRelSolFound = 0;
    bool eventSettingsRelSolLost = 0;

    QTime lapTime;
    QString version = "0.2.2";

    QLabel* versionLabel;
    QLabel* experimentDirectoryLabel;

};
#endif // MAINWINDOW_H
