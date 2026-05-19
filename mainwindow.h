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

#include "enums.h"
#include "structs.h"

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE


class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

public slots:
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

signals:
    void newData();

private:
    void getMessagesConfig();

    void fillConnectionsTable();
    void addItemToConnectionsTable(DeviceInfo info);
    bool deleteDir(const QString &dirName, bool isDeleteOnlyContents = false);
    void setupTableSize(QTableWidget* table);
    void addItemToLogTable(QString localTime, QString GNSSTime, QString event);
    Ui::MainWindow *ui;

    //контейнеры
    QMap<QString,DeviceInfo> devicesMap;
    QMap<QString,TableConnectionsFields> tableFieldsMap;
    QMap<QString, QObject*> connectionsMap;
    QList<QList<QString>> notesList;
    QMap<QString,QByteArray*> bufferMap;
    QMap<QString,EventData> eventMap;
    QMap<QString,Mess> messagesMap;
    QMap<QString,NewData> newDataMap;

    //работа с файлами
    QDomDocument connectionsDoc;
    QDomElement connectionsRootElement;
    QDomDocument eventDoc;
    QDomElement eventRootElement;
    QString experimentDirectory;

    //флаги
    bool canRead = 1;
    bool isLap = 0;
    int lapNumber = 0;
    bool eventSettingsSolFound = 0;
    bool eventSettingsSolLost = 0;
    bool eventSettingsRelSolFound = 0;
    bool eventSettingsRelSolLost = 0;

    QTime lapTime;
    QString version = "0.1.0";

};
#endif // MAINWINDOW_H
