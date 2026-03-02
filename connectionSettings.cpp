#include "connectionSettings.h"
#include "ui_connectionSettings.h"

ConnectionSettings::ConnectionSettings(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::ConnectionSettings)
{
    ui->setupUi(this);
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
        comboBaudrate->setCurrentText(parameters.at(INDEX_SERIAL_BAUDRATE));
        comboDataBits->setCurrentText(parameters.at(INDEX_SERIAL_DATA_BITS));
        comboParity->setCurrentText(parameters.at(INDEX_SERIAL_PARITY));
        comboStopBits->setCurrentText(parameters.at(INDEX_SERIAL_STOP_BITS));
        comboConnectionCount->setCurrentText(parameters.at(INDEX_SERIAL_TCP_COUNT));
        lineEditTCPNumber->setText(parameters.at(INDEX_SERIAL_TCP_PORT));
    }
    else if (protocolName == "TCP"){
        comboClientServer->setCurrentText(parameters.at(INDEX_TCP_CLIENT_SERVER));
        lineEditPort->setText(parameters.at(INDEX_TCP_PORT));
        lineEditAdress->setText(parameters.at(INDEX_TCP_ADRESS));
    }
    else if (protocolName == "CAN"){
        comboCANType->setCurrentText(parameters.at(INDEX_CAN_TYPE));
        comboBaudrate->setCurrentText(parameters.at(INDEX_CAN_BAUDRATE));
    }

}

ConnectionSettings::~ConnectionSettings()
{
    delete ui;
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

void clearGridLayout(QGridLayout* layout)
{
    if (!layout)
        return;

    QLayoutItem *item;
    while ((item = layout->takeAt(0)) != nullptr) {
        QWidget *widget = item->widget();
        if (widget) {
            widget->setParent(nullptr);
            widget->deleteLater();
        }
        delete item;
    }
}

void ConnectionSettings::toggleAdressEditable(QString string)
{
    if (string == "Клиент"){
        lineEditAdress->setEnabled(true);
    }
    else if (string == "Сервер"){
        lineEditAdress->setEnabled(false);
    }
}

void ConnectionSettings::changeSettings(QString connectionType)
{
    clearGridLayout(ui->gridLayoutSettings);
    if(connectionType == "Serial"){
        labelBaudrate            = new QLabel(this);
        labelDataBits           = new QLabel(this);
        labelTCPNumber          = new QLabel(this);
        labelEven               = new QLabel(this);
        labelStopBits           = new QLabel(this);
        labelConnectionCount    = new QLabel(this);
        comboBaudrate            = new QComboBox(this);
        comboDataBits           = new QComboBox(this);
        comboParity             = new QComboBox(this);
        comboStopBits           = new QComboBox(this);
        comboConnectionCount    = new QComboBox(this);
        lineEditTCPNumber       = new QLineEdit(this);

        labelBaudrate->setText("Baudrate");
        labelDataBits->setText("Биты данных");
        labelTCPNumber->setText("Номер TCP порта");
        labelEven->setText("Четность");
        labelStopBits->setText("Стоповые биты");
        labelConnectionCount->setText("Кол-во подключений TCP");

        int baudrate = 300;
        for(int i = 0; i < 8; i++){
            comboBaudrate->addItem(QString::number(baudrate));
            baudrate *=2;
        }
        baudrate = 57600;
        comboBaudrate->addItem(QString::number(baudrate));
        baudrate = 115200;
        for(int i = 0; i < 4; i++){
            comboBaudrate->addItem(QString::number(baudrate));
            baudrate *=2;
        }
        comboBaudrate->setEditable(true);

        comboDataBits->addItem("7");
        comboDataBits->addItem("8");

        comboParity->addItem("Нет");
        comboParity->addItem("Четное");
        comboParity->addItem("Нечетное");

        comboStopBits->addItem("1");
        comboStopBits->addItem("2");

        comboConnectionCount->addItem("1");
        comboConnectionCount->addItem("2");
        comboConnectionCount->addItem("3");
        comboConnectionCount->addItem("4");
        comboConnectionCount->addItem("5");


        ui->gridLayoutSettings->addWidget(labelBaudrate, 0, 0);
        ui->gridLayoutSettings->addWidget(labelDataBits, 1, 0);
        ui->gridLayoutSettings->addWidget(labelTCPNumber, 2, 0);
        ui->gridLayoutSettings->addWidget(comboBaudrate, 0, 1);
        ui->gridLayoutSettings->addWidget(comboDataBits, 1, 1);
        ui->gridLayoutSettings->addWidget(lineEditTCPNumber, 2, 1);
        ui->gridLayoutSettings->addWidget(labelEven, 0, 2);
        ui->gridLayoutSettings->addWidget(labelStopBits, 1, 2);
        ui->gridLayoutSettings->addWidget(labelConnectionCount, 2, 2);
        ui->gridLayoutSettings->addWidget(comboParity, 0, 3);
        ui->gridLayoutSettings->addWidget(comboStopBits, 1, 3);
        ui->gridLayoutSettings->addWidget(comboConnectionCount, 2, 3);
        ui->gridLayoutSettings->setColumnStretch(1,1);
        ui->gridLayoutSettings->setColumnStretch(3,1);
    }
    else if (connectionType == "TCP"){
        labelClientServer       = new QLabel(this);
        labelAdress             = new QLabel(this);
        labelPort               = new QLabel(this);
        comboClientServer    = new QComboBox(this);
        lineEditPort         = new QLineEdit(this);
        lineEditAdress       = new QLineEdit(this);
        labelClientServer->setText("Клиент/сервер");
        labelAdress->setText("Адрес");
        labelPort->setText("Порт");

        comboClientServer->addItem("Клиент");
        comboClientServer->addItem("Сервер");

        connect(comboClientServer, SIGNAL(currentTextChanged(QString)), this, SLOT(toggleAdressEditable(QString)));

        ui->gridLayoutSettings->addWidget(labelClientServer, 0, 0);
        ui->gridLayoutSettings->addWidget(labelPort, 1, 0);
        ui->gridLayoutSettings->addWidget(labelAdress, 2, 0);
        ui->gridLayoutSettings->addWidget(comboClientServer, 0, 1);
        ui->gridLayoutSettings->addWidget(lineEditPort, 1, 1);
        ui->gridLayoutSettings->addWidget(lineEditAdress, 2, 1);
        ui->gridLayoutSettings->setColumnStretch(1,1);


    }
    else if (connectionType == "CAN"){
        labelBaudrate            = new QLabel(this);
        labelCANType            = new QLabel(this);
        comboCANType         = new QComboBox(this);
        comboBaudrate       = new QComboBox(this);
        labelBaudrate->setText("Baudrate");
        labelCANType->setText("Тип CAN");

        int baudrate = 300;
        for(int i = 0; i < 8; i++){
            comboBaudrate->addItem(QString::number(baudrate));
            baudrate *=2;
        }
        baudrate = 57600;
        comboBaudrate->addItem(QString::number(baudrate));
        baudrate = 115200;
        for(int i = 0; i < 4; i++){
            comboBaudrate->addItem(QString::number(baudrate));
            baudrate *=2;
        }
        comboBaudrate->setEditable(true);

        comboCANType->addItem("USB-CAN");

        ui->gridLayoutSettings->addWidget(labelBaudrate, 0, 0);
        ui->gridLayoutSettings->addWidget(labelCANType, 1, 0);
        ui->gridLayoutSettings->addWidget(comboBaudrate, 0, 1);
        ui->gridLayoutSettings->addWidget(comboCANType, 1, 1);
        ui->gridLayoutSettings->setColumnStretch(1,1);
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
        if(lineEditTCPNumber->text().isEmpty()){
            title = "Не указан TCP порт!";
            text = "Введите TCP порт";
        }
        else isWarning = 0;
    }
    else if(ui->comboBoxConnectionType->currentText() == "TCP"){
        if(lineEditPort->text().isEmpty()){
            title = "Не указан TCP порт!";
            text = "Введите TCP порт";
        }
        else if(comboClientServer->currentText() == "Клиент" && lineEditAdress->text().isEmpty()){
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
        QString baudrate = comboBaudrate->currentText();
        QString dataBits = comboDataBits->currentText();
        QString TCPNumber = lineEditTCPNumber->text();
        QString parity = comboParity->currentText();
        QString stopBits = comboStopBits->currentText();
        QString TCPConnectionCount = comboConnectionCount->currentText();
        info.append(baudrate);
        info.append(dataBits);
        info.append(TCPNumber);
        info.append(parity);
        info.append(stopBits);
        info.append(TCPConnectionCount);
    }
    else if (connectionType == "CAN"){
        QString baudrate = comboBaudrate->currentText();
        QString CANType = comboCANType->currentText();
        info.append(baudrate);
        info.append(CANType);
    }
    else if (connectionType == "TCP"){
        QString clientServer = comboClientServer->currentText();
        QString port = lineEditPort->text();
        QString adress = lineEditAdress->text();
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

