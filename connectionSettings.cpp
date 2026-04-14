#include "connectionSettings.h"
#include "ui_connectionSettings.h"

ConnectionSettings::ConnectionSettings(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::ConnectionSettings)
{
    ui->setupUi(this);
    setChildrenHidden(ui->groupBoxSettings,true);
}

ConnectionSettings::ConnectionSettings(QPair<QString,QList<QString>> deviceInfo, QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::ConnectionSettings)
{
    ui->setupUi(this);
    QString protocolName = deviceInfo.first;
    QList<QString> parameters = deviceInfo.second;
    ui->lineEditName->setText(parameters.at(INDEX_GENERAL_ID));
    ui->comboBoxConnectionType->setCurrentText(protocolName);
    ui->comboBoxDevice->setCurrentText(parameters.at(INDEX_GENERAL_DEVICE_TYPE));
    ui->comboBoxProtocol->setCurrentText(parameters.at(INDEX_GENERAL_PROTOCOL));
    ui->comboBoxConnectionType->setEnabled(false);
    ui->comboBoxDevice->setEnabled(false);
    ui->comboBoxProtocol->setEnabled(false);
    ui->lineEditName->setEnabled(false);
    changeSettings(protocolName);
    if (protocolName == "Serial"){
        ui->lineSerialPort->setText(parameters.at(INDEX_SERIAL_PORT));
        ui->comboSerialBaud->setCurrentText(parameters.at(INDEX_SERIAL_BAUDRATE));
        ui->comboDataBits->setCurrentText(parameters.at(INDEX_SERIAL_DATA_BITS));
        ui->comboParity->setCurrentText(parameters.at(INDEX_SERIAL_PARITY));
        ui->comboStopBits->setCurrentText(parameters.at(INDEX_SERIAL_STOP_BITS));
        ui->comboSerialConnNum->setCurrentText(parameters.at(INDEX_SERIAL_TCP_COUNT));
        ui->lineSerialTCPport->setText(parameters.at(INDEX_SERIAL_TCP_PORT));
    }
    else if (protocolName == "TCP"){
        ui->comboTCPclientServer->setCurrentText(parameters.at(INDEX_TCP_CLIENT_SERVER));
        ui->lineTCPport->setText(parameters.at(INDEX_TCP_PORT));
        ui->lineTCPaddr->setText(parameters.at(INDEX_TCP_ADRESS));
    }
    else if (protocolName == "CAN"){
        ui->comboCANtype->setCurrentText(parameters.at(INDEX_CAN_TYPE));
        ui->comboCANBaud->setCurrentText(parameters.at(INDEX_CAN_BAUDRATE));
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

QPair<QString,QList<QString>> ConnectionSettings::getSettings()
{
    return this->settings;
}

void ConnectionSettings::saveSettings()
{
    settings.first.clear();
    settings.second.clear();
    QString connectionType = ui->comboBoxConnectionType->currentText();
    qDebug() << connectionType;
    QList<QString> info;
    QString id = ui->lineEditName->text();
    QString device = ui->comboBoxDevice->currentText();
    QString protocol = ui->comboBoxProtocol->currentText();
    info.append(id);
    info.append(device);
    info.append(protocol);
    if (connectionType == "Serial"){
        QString port = ui->lineSerialPort->text();
        QString baudrate = ui->comboSerialBaud->currentText();
        QString dataBits = ui->comboDataBits->currentText();
        QString TCPNumber = ui->lineSerialTCPport->text();
        QString parity = ui->comboParity->currentText();
        QString stopBits = ui->comboStopBits->currentText();
        QString TCPConnectionCount = ui->comboSerialConnNum->currentText();
        info.append(port);
        info.append(baudrate);
        info.append(dataBits);
        info.append(TCPNumber);
        info.append(parity);
        info.append(stopBits);
        info.append(TCPConnectionCount);
    }
    else if (connectionType == "CAN"){
        QString baudrate = ui->comboCANBaud->currentText();
        QString CANType = ui->comboCANtype->currentText();
        info.append(baudrate);
        info.append(CANType);
    }
    else if (connectionType == "TCP"){
        QString clientServer = ui->comboTCPclientServer->currentText();
        QString port = ui->lineTCPport->text();
        QString adress = ui->lineTCPaddr->text();
        info.append(clientServer);
        info.append(port);
        info.append(adress);
    }
    else{
        settings.first.clear();
        settings.second.clear();
        return;
    }

    settings.first = connectionType;
    settings.second = info;
}

