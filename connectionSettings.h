#ifndef CONNECTIONSETTINGS_H
#define CONNECTIONSETTINGS_H

#include <QDialog>
#include <QDebug>
#include <QComboBox>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QPair>
#include <QList>

#include "enums.h"


QT_BEGIN_NAMESPACE
namespace Ui { class ConnectionSettings; }
QT_END_NAMESPACE

class ConnectionSettings : public QDialog
{
    Q_OBJECT

public:
    ConnectionSettings(QWidget *parent = nullptr);
    ConnectionSettings(QPair<QString,QList<QString>> deviceInfo, QWidget *parent = nullptr);
    ~ConnectionSettings();


    QPair<QString,QList<QString>> getSettings();

public slots:
    void changeProtocolComboBox(QString device);
    void changeSettings(QString connectionType);
    void toggleAdressEditable(QString string);
    void saveSettings();
    void checkFields();

private:
    Ui::ConnectionSettings *ui;
    QPair<QString,QList<QString>> settings;
    QLabel* labelDataBits;
    QLabel* labelTCPNumber;
    QLabel* labelEven;
    QLabel* labelStopBits;
    QLabel* labelConnectionCount;
    QComboBox* comboBaudrate;
    QComboBox* comboDataBits;
    QComboBox* comboParity;
    QComboBox* comboStopBits;
    QComboBox* comboConnectionCount;
    QLineEdit* lineEditTCPNumber;
    QLabel* labelClientServer;
    QLabel* labelAdress;
    QLabel* labelPort;
    QComboBox* comboClientServer;
    QLineEdit* lineEditAdress;
    QLineEdit* lineEditPort;
    QLabel* labelBaudrate;
    QLabel* labelCANType;
    QComboBox* comboCANType;
};
#endif // CONNECTIONSETTINGS_H
