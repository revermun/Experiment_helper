#include "connectionSettings.h"
#include "ui_connectionSettings.h"

ConnectionSettings::ConnectionSettings(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::ConnectionSettings)
{
    ui->setupUi(this);
    setChildrenHidden(ui->groupBoxSettings,true);
}

ConnectionSettings::ConnectionSettings(DeviceInfo deviceInfo, QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::ConnectionSettings)
{
    ui->setupUi(this);
    QString connectionType = deviceInfo.connType;
    ui->lineEditName->setText(deviceInfo.ID);
    ui->comboBoxConnectionType->setCurrentText(connectionType);
    ui->comboBoxDevice->setCurrentText(deviceInfo.deviceType);
    ui->comboBoxProtocol->setCurrentText(deviceInfo.protocol);
    ui->comboBoxConnectionType->setEnabled(false);
    ui->comboBoxDevice->setEnabled(false);
    ui->comboBoxProtocol->setEnabled(false);
    ui->lineEditName->setEnabled(false);
    changeSettings(connectionType);
    if (connectionType == "Serial"){
        ui->comboSerialPort->setCurrentText     (deviceInfo.serialInfo.port);
        ui->comboSerialBaud->setCurrentText     (QString::number(deviceInfo.serialInfo.baudrate));
        ui->comboDataBits->setCurrentText       (QString::number(deviceInfo.serialInfo.dataBits));
        ui->comboParity->setCurrentText         (deviceInfo.serialInfo.parity);
        ui->comboStopBits->setCurrentText       (QString::number(deviceInfo.serialInfo.stopBits));
        ui->comboSerialConnNum->setCurrentText  (QString::number(deviceInfo.serialInfo.tcpCount));
        ui->lineSerialTCPport->setText          (deviceInfo.serialInfo.tcpPort);
    }
    else if (connectionType == "TCP"){
        ui->comboTCPclientServer->setCurrentText(deviceInfo.tcpInfo.clientServer);
        ui->lineTCPport->setText                (deviceInfo.tcpInfo.port);
        ui->lineTCPaddr->setText                (deviceInfo.tcpInfo.adress);
    }
    else if (connectionType == "CAN"){
        ui->comboCANtype->setCurrentText        (deviceInfo.canInfo.type);
        ui->comboCANBaud->setCurrentText        (QString::number(deviceInfo.canInfo.baudrate));
    }

}

ConnectionSettings::~ConnectionSettings()
{
    delete ui;
}

void ConnectionSettings::setChildrenHidden(QObject* parent, bool isHidden){
    foreach (QObject* object, parent->children()) {
        if(qobject_cast<QWidget*>(object)){
            QWidget *widget = qobject_cast<QWidget*>(object);
            widget->setHidden(isHidden);
        }
    }
}


void ConnectionSettings::changeProtocolComboBox(QString device)
{
    ui->comboBoxProtocol->clear();
    if (device == "Приемник"){
        ui->comboBoxProtocol->addItem("Ublox");
        ui->comboBoxProtocol->addItem("Unicore");
    }
    else if (device == "IMU"){
        ui->comboBoxProtocol->addItem("WitMotion");
        ui->comboBoxProtocol->addItem("BS-IC24");
    }
    else if (device == "Другое"){
        ui->comboBoxProtocol->addItem("Ublox");
        ui->comboBoxProtocol->addItem("Unicore");
        ui->comboBoxProtocol->addItem("WitMotion");
        ui->comboBoxProtocol->addItem("BS-IC24");
    }
}

void ConnectionSettings::toggleAdressEditable(QString string)
{
    if (string == "Клиент"){
        ui->lineTCPaddr->setEnabled(true);
    }
    else if (string == "Сервер"){
        ui->lineTCPaddr->setEnabled(false);
    }
}

void ConnectionSettings::changeSettings(QString connectionType)
{
    setChildrenHidden(ui->groupBoxSettings,true);
    if (connectionType == "Serial"){
        setChildrenHidden(ui->frameSerial,false);
        ui->frameSerial->setHidden(false);
        ui->comboSerialPort->clear();
        const auto serialPortInfos = QSerialPortInfo::availablePorts();
        for (const QSerialPortInfo &portInfo : serialPortInfos) {
            ui->comboSerialPort->addItem(portInfo.portName());
        }
    }
    else if (connectionType == "CAN"){
        setChildrenHidden(ui->frameCAN,false);
        ui->frameCAN->setHidden(false);
    }
    else if (connectionType == "TCP"){
        setChildrenHidden(ui->frame,false);
        ui->frame->setHidden(false);
    }
}

void ConnectionSettings::checkFields(){
    QString title;
    QString text;
    bool isWarning = 1;
    if(ui->lineEditName->text().isEmpty()){
        title = "Поле id пустое!";
        text = "Введите название устройства";
    }
    else if(ui->comboBoxConnectionType->currentText().isEmpty()){
        title = "Не выбран тип подключения!";
        text = "Выберите тип подключения";
    }
    else if(ui->comboBoxConnectionType->currentText() == "Serial"){
        if(ui->lineSerialTCPport->text().isEmpty()){
            title = "Не указан TCP порт!";
            text = "Введите TCP порт";
        }
        else isWarning = 0;
    }
    else if(ui->comboBoxConnectionType->currentText() == "TCP"){
        if(ui->lineTCPport->text().isEmpty()){
            title = "Не указан TCP порт!";
            text = "Введите TCP порт";
        }
        else if(ui->comboTCPclientServer->currentText() == "Клиент" && ui->lineTCPaddr->text().isEmpty()){
            title = "Не указан адрес!";
            text = "Введите адрес";
        }
        else isWarning = 0;
    }
    else isWarning = 0;

    if(isWarning){
        QMessageBox::warning(this, title, text);
        return;
    }
    else {
        saveSettings();
        this->accept();
    }
}

DeviceInfo ConnectionSettings::getSettings()
{
    return this->settings;
}

void ConnectionSettings::saveSettings()
{
    DeviceInfo deviceInfo;
    QString id = ui->lineEditName->text();
    QString device = ui->comboBoxDevice->currentText();
    QString protocol = ui->comboBoxProtocol->currentText();
    QString connectionType = ui->comboBoxConnectionType->currentText();
    deviceInfo.ID = id;
    deviceInfo.deviceType = device;
    deviceInfo.protocol = protocol;
    deviceInfo.connType = connectionType;
    if (connectionType == "Serial"){
        QString port = ui->comboSerialPort->currentText();
        QString baudrate = ui->comboSerialBaud->currentText();
        QString dataBits = ui->comboDataBits->currentText();
        QString parity = ui->comboParity->currentText();
        QString stopBits = ui->comboStopBits->currentText();
        QString tcpPort = ui->lineSerialTCPport->text();
        QString tcpCount = ui->comboSerialConnNum->currentText();
        deviceInfo.serialInfo.port = port;
        deviceInfo.serialInfo.baudrate = baudrate.toInt();
        deviceInfo.serialInfo.dataBits = dataBits.toInt();
        deviceInfo.serialInfo.parity = parity;
        deviceInfo.serialInfo.stopBits = stopBits.toInt();
        deviceInfo.serialInfo.tcpCount = tcpCount.toInt();
        deviceInfo.serialInfo.tcpPort = tcpPort;
    }
    else if (connectionType == "CAN"){
        QString baudrate = ui->comboCANBaud->currentText();
        QString type = ui->comboCANtype->currentText();
        deviceInfo.canInfo.baudrate = baudrate.toInt();
        deviceInfo.canInfo.type = type;
    }
    else if (connectionType == "TCP"){
        QString clientServer = ui->comboTCPclientServer->currentText();
        QString port = ui->lineTCPport->text();
        QString adress = ui->lineTCPaddr->text();
        deviceInfo.tcpInfo.clientServer = clientServer;
        deviceInfo.tcpInfo.port = port;
        deviceInfo.tcpInfo.adress = adress;
    }

    this->settings = deviceInfo;
}

