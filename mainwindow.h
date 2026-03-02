#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QMap>
#include <QList>
#include <QDomDocument>
#include <QFileDialog>
#include <QCheckBox>
#include <QSerialPort>

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

enum tableConnectionsIndexes{
    INDEX_CONN_TABLE_ID,
    INDEX_CONN_TABLE_TYPE,
    INDEX_CONN_TABLE_TCP_PORT,
    INDEX_CONN_TABLE_ON_OFF,
    INDEX_CONN_TABLE_DATA
};

struct tableConnectionsFields{
    QString ID;
    QString connectionType;
    QString TCPPort;
    int onOff;
    int data;
};



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

private:
    void fillConnectionsTable();
    void addItemToConnectionsTable(QString protocol, QList<QString> parameters);
    bool deleteDir(const QString &dirName, bool isDeleteOnlyContents = false);
    // QSerialPort connection;
    Ui::MainWindow *ui;

    //контейнеры
    QMap<QString,QPair<QString,QList<QString>>> devicesMap;
    QMap<QString, QSerialPort*> connectionsMap;
    QList<QList<QString>> notesList;

    //работа с файлами
    QDomDocument connectionsDoc;
    QDomElement connectionsRootElement;
    QString experimentDirectory;

    //флаги
    bool isLap = 0;
    int lapNumber = 0;
    bool eventSettingsSolutionFound = 0;
    bool eventSettingsSolutionLost = 0;
    bool eventSettingsETC = 0;

};
#endif // MAINWINDOW_H
