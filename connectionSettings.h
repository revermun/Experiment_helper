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

enum generalIndexes{
    INDEX_GENERAL_ID,
    INDEX_GENERAL_DEVICE_TYPE,
    INDEX_GENERAL_PROTOCOL
};

enum serialIndexes{
    INDEX_SERIAL_ID,
    INDEX_SERIAL_DEVICE_TYPE,
    INDEX_SERIAL_PROTOCOL,
    INDEX_SERIAL_BAUDRATE,
    INDEX_SERIAL_DATA_BITS,
    INDEX_SERIAL_TCP_PORT,
    INDEX_SERIAL_PARITY,
    INDEX_SERIAL_STOP_BITS,
    INDEX_SERIAL_TCP_COUNT
};

enum CANIndexes{
    INDEX_CAN_ID,
    INDEX_CAN_DEVICE_TYPE,
    INDEX_CAN_PROTOCOL,
    INDEX_CAN_BAUDRATE,
    INDEX_CAN_TYPE
};

enum TCPIndexes{
    INDEX_TCP_ID,
    INDEX_TCP_DEVICE_TYPE,
    INDEX_TCP_PROTOCOL,
    INDEX_TCP_CLIENT_SERVER,
    INDEX_TCP_PORT,
    INDEX_TCP_ADRESS
};

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
