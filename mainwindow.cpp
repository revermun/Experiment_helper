#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "connectionSettings.h"
#include "notesdialog.h"
#include "startstopactionsdialog.h"
#include "dataandgraphsdialog.h"
#include "experimentconfigurationdialog.h"
#include "deviceconfigurationsdialog.h"
#include "eventsettingsdialog.h"
#include "ubloxparser.h"
#include "unicoreparser.h"
#include "convertors.h"
#include "pickdirectorydialog.h"
#include "presetsettingsdialog.h"



/// TODO: Пресеты
/// Что это?
/// Пресет - это совокупность файла схемы установки (mounting) вместе с файлами конфигурации устройтв
/// Зачем это?
/// Чтобы во время эксперимента можно было менять его условия без лишних действий
/// Как это сделать?
/// 1) Реализовать конфигурацию устройств по файлам
/// 2) Реализовать сохранение пресета (сохранение текущей конфигурации устройств и файла mounting)
///    в отдельную папку в новой папке Lap_presets
/// 3) Реализовать функционал кнопки Инициализации при старте/остановке
///
/// TODO: Файлы конфигурации устройств
/// Что это?
/// Это файлы, содержащие набор команд, необходимых для отправки на устройства.
/// Команды должны соответствовать протоколу устройства и идти друг за другом через \n
/// Примеры:
/// UM;MODE;ROVER AUTOMOTIVE для Unicore
/// UBX;cfg-msg;01,01,0,0,0,1,0,0 для Ublox (После названия через запятую задаются значения полей сообщения)
///
/// TODO: Сохранение пресета
/// Пресеты должны сохранятся в следующем виде
/// (п) Lap_presets
///         (п)<preset_name>
///             (п)device_configurations
///             -Preset_Info (устройства пресета)
///             -Mounting
///             -Connections
///             -Start_and_stop_actions
/// При выборе "Сохранить" открывается окно редактирования пресета
/// В нем должно быть название, описание, выбор конфигурационных файлов или редактор конфигураций
/// устройств.
/// TODO: Множественное подключение и отключение



void MainWindow::setupTableSize(QTableWidget* table) {
    // Автоматическая подгонка столбцов
    table->resizeColumnsToContents();
    table->resizeRowsToContents();

    // Отключаем скроллбары (если таблица небольшая)
    table->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    table->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

    // Вычисляем и устанавливаем размер
    table->setFixedHeight(table->verticalHeader()->length() + table->horizontalHeader()->height() + table->frameWidth() * 2);
    table->setMinimumWidth(table->horizontalHeader()->length() + table->verticalHeader()->width() + table->frameWidth() * 2);
}

template<typename K, typename V>
void dumpSimpleMap(const QMap<K, V> &m, int indent = 0)
{
    QString prefix(indent, ' ');
    for (auto it = m.cbegin(); it != m.cend(); ++it) {
        // qDebug() << prefix << it.key() << "->" << it.value();
    }
}

template<typename K, typename InnerK, typename InnerV>
void dumpNestedMap(const QMap<K, QMap<InnerK, InnerV>> &m, int indent = 0)
{
    QString prefix(indent, ' ');
    for (auto it = m.cbegin(); it != m.cend(); ++it) {
        // qDebug() << prefix << "Key:" << it.key();
        dumpSimpleMap(it.value(), indent + 4);
    }
}

bool MainWindow::deleteDir(const QString &dirName, bool isDeleteOnlyContents)
{
    QDir directory(dirName);
    if (!directory.exists())
    {
        return true;
    }

    QString srcPath = QDir::toNativeSeparators(dirName);
    if (!srcPath.endsWith(QDir::separator()))
        srcPath += QDir::separator();

    QStringList fileNames = directory.entryList(QDir::AllEntries | QDir::NoDotAndDotDot | QDir::Hidden);
    bool error = false;
    for (QStringList::size_type i=0; i != fileNames.size(); ++i)
    {
        QString filePath = srcPath + fileNames.at(i);
        QFileInfo fileInfo(filePath);
        if (fileInfo.isFile() || fileInfo.isSymLink())
        {
            QFile::setPermissions(filePath, QFile::WriteOwner);
            if (!QFile::remove(filePath))
            {
                // qDebug() << "remove file" << filePath << " failed!";
                error = true;
            }
        }
        else if (fileInfo.isDir())
        {
            if (!deleteDir(filePath))
            {
                error = true;
            }
        }
    }
    if (!isDeleteOnlyContents){
        if (!directory.rmdir(QDir::toNativeSeparators(directory.path())))
        {
            // qDebug() << "remove dir" << directory.path() << " failed!";
            error = true;
        }
    }
    return !error;
}

bool MainWindow::openPickDirectoryDialog(){
    pickDirectoryDialog pdd(this);
    if (pdd.exec() == QDialog::Accepted){
        QString dir = pdd.getDir();
        if (dir.isEmpty()) return false;
        loadExperiment(dir);
        return true;
    }
    return false;
}

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    versionLabel = new QLabel();
    experimentDirectoryLabel = new QLabel();

    if(!openPickDirectoryDialog()){
        QWidgetList allWidgets = QApplication::allWidgets();
        foreach (QWidget* widget, allWidgets) {
            QPushButton* button = qobject_cast<QPushButton*>(widget);
            if (button) {
                button->setEnabled(false);
            }
        }
    }

    ui->tableWidgetConnections->setEditTriggers(QAbstractItemView::NoEditTriggers);

    currentPreset.title = "Default";
    currentPreset.description = "";
    updatePreset();

    connectionsRootElement = connectionsDoc.createElement("Connections");
    connectionsDoc.appendChild(connectionsRootElement);

    eventRootElement = eventDoc.createElement("Events");
    eventDoc.appendChild(eventRootElement);

    getMessagesConfig();

    QTimer *timer = new QTimer(this);
    connect(timer, SIGNAL(timeout()), this, SLOT(indicateData()));
    connect(timer, SIGNAL(timeout()), this, SLOT(changeLapTimer()));
    timer->start(1);
    versionLabel->setText(tr("Версия ПО: ") + this->version);
    statusBar()->addWidget(versionLabel);
    statusBar()->addWidget(experimentDirectoryLabel);
}

MainWindow::~MainWindow()
{
    foreach (QString key, connectionsMap.keys()) {
        QObject* connection = connectionsMap[key];
        if (qobject_cast<QIODevice*>(connection)){
            QIODevice* ioCon = qobject_cast<QIODevice*>(connection);
            ioCon->close();
        }
    }
    delete ui;
}

void MainWindow::updatePreset()
{
    ui->labelPresetTitle->setText(currentPreset.title);
    ui->labelPresetDescription->setText(currentPreset.description);
}

void MainWindow::getMessagesConfig()
{
    QString dir = QCoreApplication::applicationDirPath()+"/config_messages.xml";
    QFile file(dir);
    if (!file.open(QIODevice::ReadOnly)){
        QMessageBox::warning(this, "Ошибка", "Не обнаружен конфигурационный файл config_messages.xml\n"
                                             "Добавьте его в папку приложения и перезапустите!\n"
                                             "Если у вас нет файла, то спросите его у разработчика.");
        return;
    }
    QDomDocument doc("document");
    if (!doc.setContent(&file)) {
        file.close();
        return;
    }
    file.close();

    QDomElement docElem = doc.documentElement();
    QDomNode messageXml = docElem.firstChild();
    while(!messageXml.isNull()) {
        QDomElement e = messageXml.toElement();
        if(!e.isNull()) {
            QString messageName = e.tagName();
            Mess message;
            message.name = messageName.replace('_',' ');
            message.description = e.attribute("description");
            message.id = e.attribute("id");
            message.type = e.attribute("type");
            message.protocol = e.attribute("protocol");
            QDomNode fieldXml = messageXml.firstChild();
            QMap<QString,Mess::Field> fields;
            int index = 0;
            while(!fieldXml.isNull()) {
                QDomElement i = fieldXml.toElement();
                if(!i.isNull()) {
                    QString fieldName = i.tagName();

                    Mess::Field field;

                    field.name = fieldName;
                    field.index = index;
                    field.full_name = i.attribute("name");
                    field.type = i.attribute("type");
                    field.isRepeated = i.attribute("repeated").toInt();
                    field.size = 0;
                    field.offset = 0;
                    field.min_value = 0;
                    field.max_value = 0;
                    field.units = QString();
                    field.scale = 1.0;

                    QString sizeAttr = i.attribute("size");
                    if (!sizeAttr.isEmpty()) field.size = sizeAttr.toInt();

                    QString offsetAttr = i.attribute("offset");
                    if (!offsetAttr.isEmpty()) field.offset = offsetAttr.toInt();

                    QString minAttr = i.attribute("min_value");
                    if (!minAttr.isEmpty()) field.min_value = minAttr.toInt();

                    QString maxAttr = i.attribute("max_value");
                    if (!maxAttr.isEmpty()) field.max_value = maxAttr.toInt();

                    QString unitsAttr = i.attribute("units");
                    if (!unitsAttr.isEmpty()) field.units = unitsAttr;

                    QString scaleAttr = i.attribute("scale");
                    if (!scaleAttr.isEmpty()) field.scale = scaleAttr.toDouble();

                    fields.insert(fieldName, field);
                    index++;
                }
                fieldXml = fieldXml.nextSibling();
            }
            message.fields = fields;
            messagesMap[messageName] = message;
        }
        messageXml = messageXml.nextSibling();
    }
}


void MainWindow::addItemToConnectionsTable(DeviceInfo info)
{
    int rowCount = ui->tableWidgetConnections->rowCount();
    ui->tableWidgetConnections->setRowCount(rowCount+1);
    QString deviceName = info.ID;
    QString connType = info.connType;
    TableConnectionsFields fields;
    fields.row = rowCount;
    fields.ID = new QTableWidgetItem(deviceName);
    fields.connectionType = new QTableWidgetItem(connType);
    fields.TCPPort = new QTableWidgetItem();
    fields.onOff = new QTableWidgetItem;
    fields.data = new QTableWidgetItem;
    fields.ID->setText(deviceName);
    fields.connectionType->setText(connType);
    if (connType == "Serial"){
        if (info.serialInfo.isTranslating){
            QString TCPPort = QString::number(info.serialInfo.tcpPort);
            QString string = QString("%1 (%2 из %3)").arg(TCPPort).arg(0).arg(QString::number(info.serialInfo.tcpCount));
            fields.TCPPort->setText(string);
        }
    }
    else if (connType == "CAN"){
        // ничего
    }
    else if (connType == "TCP"){
        uint16_t port =  info.tcpInfo.port;

        fields.TCPPort->setText(QString::number(port));
    }
    ui->tableWidgetConnections->setItem(rowCount,INDEX_CONN_TABLE_ID,fields.ID);
    ui->tableWidgetConnections->setItem(rowCount,INDEX_CONN_TABLE_TYPE,fields.connectionType);
    ui->tableWidgetConnections->setItem(rowCount,INDEX_CONN_TABLE_TCP_PORT,fields.TCPPort);
    fields.onOff->setBackground(QBrush(QColor(255,0,0)));
    ui->tableWidgetConnections->setItem(rowCount, INDEX_CONN_TABLE_ON_OFF, fields.onOff);
    fields.data->setBackground(QBrush(QColor(0,100,0)));
    ui->tableWidgetConnections->setItem(rowCount, INDEX_CONN_TABLE_DATA, fields.data);
    tableFieldsMap[deviceName] = fields;
    setupTableSize(ui->tableWidgetConnections);
}

void MainWindow::onNewBridgeConnection(){
    SerialToTcpBridge* bridge = qobject_cast<SerialToTcpBridge*>(sender());
    foreach (QString key, bridgeMap.keys()) {
        if (bridge == bridgeMap[key]){
            DeviceInfo info = devicesMap[key];
            QString string = QString("%1 (%2 из %3)").arg(info.serialInfo.tcpPort).arg(bridge->getConCount()).arg(QString::number(info.serialInfo.tcpCount));
            tableFieldsMap[key].TCPPort->setText(string);
            return;
        }
    }
}

void MainWindow::fillConnectionsTable()
{
    ui->tableWidgetConnections->clearContents();
    ui->tableWidgetConnections->setRowCount(0);
    for (const auto &key: devicesMap.keys()) {
        DeviceInfo info = devicesMap.value(key);
        addItemToConnectionsTable(info);
    }
}


void MainWindow::loadPreset(QString dir){
    if (dir == "") {return;}
    /// Меняем текущий пресет на этот:
    QString title = dir.split('/').last();
    QFile infoFile(dir + "/Preset_info.txt");
    QString description;
    if (infoFile.open(QIODevice::ReadOnly)){
        QTextStream stream(&infoFile);
        stream.setCodec("UTF-8");
        description = stream.readAll();
    }
    Preset preset;
    preset.title = title;
    preset.description = description;
    /// заменяем файлы эксперимента на файлы пресета
    QString experimentConfigDir = experimentDirectory + "/Configurations/Experiment_configurations";
    QString presetConnectionsDir = dir + "/Connections.yaml";
    QString presetMountingDir = dir + "/Mounting.yaml";
    QFile presetConnectionsFile(presetConnectionsDir);
    QFile presetMountingFile(presetMountingDir);
    QString experimentConnectionsDir = experimentConfigDir + "/Experiment_connections.yaml";
    QFile experimentConnectionsFile(experimentConnectionsDir);
    QString experimentMountingDir = experimentConfigDir + "/Experiment_mounting.yaml";
    QFile experimentMountingFile(experimentMountingDir);
    if (presetConnectionsFile.open(QFile::ReadWrite)){
        if (experimentConnectionsFile.open(QFile::ReadWrite)) experimentConnectionsFile.remove();
        presetConnectionsFile.copy(experimentConnectionsDir);
        presetConnectionsFile.close();
        preset.connectionsMap = experimentConfigurationDialog::getConnectionsFromFile(presetConnectionsDir);
        preset.devicesMap = experimentConfigurationDialog::getDevicesFromFile(presetConnectionsDir);
    }
    if (presetConnectionsFile.open(QFile::ReadWrite)){
        if (experimentMountingFile.open(QFile::ReadWrite)) experimentMountingFile.remove();
        presetMountingFile.copy(experimentMountingDir);
        presetConnectionsFile.close();
        preset.mountingMap = experimentConfigurationDialog::getMountingFromFile(presetMountingDir);
    }
    currentPreset = preset;
    updatePreset();
}

void MainWindow::loadConnections(QString connFileDir){
    if (connFileDir == "") {return;}
    QFile file(connFileDir);
    if (!file.open(QIODevice::ReadWrite)) return;
    QDomDocument doc("document");
    file.close();
    if (!doc.setContent(&file)) return;

    QDomElement docElem = doc.documentElement();
    QDomNode deviceXml = docElem.firstChild();
    while(!deviceXml.isNull()) {
        DeviceInfo info;
        QDomElement e = deviceXml.toElement();
        QString deviceName = e.tagName();
        QDomElement parametersXml = deviceXml.firstChild().toElement();
        QString connType = parametersXml.tagName();
        QString transferProtocol =  parametersXml.attribute("transfer_protocol");
        QString deviceType =        parametersXml.attribute("device_type");
        info.ID = deviceName;
        info.connType = connType;
        info.protocol = transferProtocol;
        info.deviceType = deviceType;
        if (connType == "Serial"){
            QString port =              parametersXml.attribute("Serial_port");
            QString baudrate =          parametersXml.attribute("baudrate");
            QString dataBits =          parametersXml.attribute("data_bits");
            QString tcpPort =           parametersXml.attribute("TCP_port_number");
            QString parity =            parametersXml.attribute("parity");
            QString isTranslating =     parametersXml.attribute("is_translating");
            QString stopBits =          parametersXml.attribute("stop_bits");
            QString tcpCount =          parametersXml.attribute("TCP_connections_number");
            info.serialInfo.port = port;
            info.serialInfo.baudrate = baudrate.toInt();
            info.serialInfo.dataBits = dataBits.toInt();
            info.serialInfo.parity = parity;
            info.serialInfo.stopBits = stopBits.toInt();
            info.serialInfo.isTranslating = isTranslating.toInt();
            info.serialInfo.tcpCount = tcpCount.toInt();
            info.serialInfo.tcpPort = tcpPort.toUInt();
        }
        else if (connType == "CAN"){
            QString baudrate =           parametersXml.attribute("baudrate");
            QString type =           parametersXml.attribute("CAN_type");

            info.canInfo.baudrate = baudrate.toInt();
            info.canInfo.type = type;
        }
        else if (connType == "TCP"){
            QString clientServer =      parametersXml.attribute("source");
            QString port =              parametersXml.attribute("port_number");
            QString adress =            parametersXml.attribute("adress");
            info.tcpInfo.clientServer = clientServer;
            info.tcpInfo.port = port.toUInt();
            info.tcpInfo.adress = adress;
        }

        if(!devicesMap.contains(deviceName)){
            devicesMap[deviceName] = info;
        }
        deviceXml = deviceXml.nextSibling();
    }
    fillConnectionsTable();

    setupTableSize(ui->tableWidgetConnections);
}

void MainWindow::loadEvents(QString eventFileDir)
{
    if (eventFileDir.isEmpty()) return;
    QFile eventFile(eventFileDir);
    eventFile.open(QIODevice::ReadWrite);
    QDomDocument eventDoc("document");
    eventFile.close();
    eventDoc.setContent(&eventFile);

    QDomElement docElem = eventDoc.documentElement();
    QDomNode eventXml = docElem.firstChild();
    while(!eventXml.isNull()) {
        QDomElement e = eventXml.toElement();
        QString eventName = e.tagName().replace('_',' ');
        EventData event;
        event.name                      = e.attribute("name");
        event.device                    = e.attribute("device");
        event.protocol                  = e.attribute("protocol");
        event.message                   = e.attribute("message");
        event.messageId                 = e.attribute("messageId");
        event.fieldName                 = e.attribute("fieldName");
        event.field                     = e.attribute("field").toInt();
        event.fieldType                 = e.attribute("fieldType");
        event.text                      = e.attribute("text");
        event.intTriggers.isGreater     = e.attribute("isGreater").toInt();
        event.intTriggers.isLesser      = e.attribute("isLesser").toInt();
        event.intTriggers.isEqual       = e.attribute("isEqual").toInt();
        event.intTriggers.threshhold    = e.attribute("threshhold").toInt();
        event.bitmapTriggers.startBit   = e.attribute("startBit").toInt();
        event.bitmapTriggers.endBit     = e.attribute("endBit").toInt();
        event.bitmapTriggers.bitValue   = e.attribute("bitValue").toInt();
        event.bitmapTriggers.isEqual    = e.attribute("bitIsEqual").toInt();
        event.charTriggers.charValue    = e.attribute("charValue");
        event.status                    = e.attribute("status").toInt();
        if(!eventMap.contains(eventName)){
            eventMap[eventName] = event;
        }
        eventXml = eventXml.nextSibling();
    }
}
bool MainWindow::exploreExperiment(){
    QString dir = QFileDialog::getExistingDirectory(this, tr("Open Directory"),
                                                    "/home",
                                                    QFileDialog::ShowDirsOnly
                                                        | QFileDialog::DontResolveSymlinks);
    if (dir.isEmpty()) return false;
    this->experimentDirectory = dir;
    return true;
}

void MainWindow::changeExperimentDirectoryLabel(){
    QString dir = this->experimentDirectory;
    QStringList dirElements = dir.split('/');
    QString labelStr;
    if (dirElements.count() > 4) labelStr = QString("Директория эксперимента: %1/.../%2/%3/%4")
                       .arg(dirElements.at(0))
                       .arg(dirElements.at(dirElements.count()-3))
                       .arg(dirElements.at(dirElements.count()-2))
                       .arg(dirElements.at(dirElements.count()-1));
    else labelStr = dir;
    experimentDirectoryLabel->setText(labelStr);
}

bool MainWindow::loadExperiment(QString dir)
{
    if (dir == "") {return false;}
    this->experimentDirectory = dir;
    changeExperimentDirectoryLabel();
    this->devicesMap.clear();
    this->eventMap.clear();
    ///Чтение конфига устройств и заполнение таблицы

    QString connectionsConfigDir = dir + '/' + "Configurations" + '/' + "connections.xml";
    loadConnections(connectionsConfigDir);

    QString eventsConfigDir = dir + '/' + "Configurations" + '/' + "events.xml";
    loadEvents(eventsConfigDir);

    notesDialog nD(experimentDirectory,isLap,this);
    notesList = nD.getNotes();
    // qDebug() << notesList;
    QWidgetList allWidgets = QApplication::allWidgets();
    foreach (QWidget* widget, allWidgets) {
        QPushButton* button = qobject_cast<QPushButton*>(widget);
        if (button) {
            button->setEnabled(true);
        }
    }
    QDir experimentDir(experimentDirectory);
    experimentDir.mkpath(experimentDirectory + "/Configurations/Experiment_configurations");
    experimentDir.mkpath(experimentDirectory + "/Configurations/Device_configurations");
    experimentDir.mkpath(experimentDirectory + "/Lap_presets");
    return true;
}

void MainWindow::addConnectionFromFile()
{
    QString dir = QFileDialog::getOpenFileName(this,
                                               tr(""), experimentDirectory, tr("Connection file (*.xml)"));
    loadConnections(dir);
}


void MainWindow::savePreset(QString dir){
    if (dir.isEmpty()) dir = experimentDirectory;
    /// Открываем диалог
    Preset preset = currentPreset;
    /// После подтверждения создаем папку с названием из диалога
    QString presetDir = QString(dir + "/Lap_presets/%1").arg(preset.title);
    QDir experimentDir(dir);
    experimentDir.mkpath(presetDir);
    /// сохраняем в Preset_info поля диалога
    QString presetInfoDir = presetDir + "/Preset_info.txt";
    QFile file(presetInfoDir);
    file.open(QIODevice::ReadWrite);

    QTextStream stream(&file);
    stream.setCodec("UTF-8");
    stream << preset.description;
    file.close();
    /// сохраняем mounting
    QString mountingDir = presetDir + "/Mounting.yaml";
    QString connectionsDir = presetDir + "/Connections.yaml";
    experimentConfigurationDialog experimentConfigDialog(experimentDirectory, this);
    experimentConfigDialog.saveMounting(mountingDir);
    experimentConfigDialog.saveConnections(connectionsDir);

}

void MainWindow::saveConnections(QString dir){
    if (dir.isEmpty()) dir = experimentDirectory;
    connectionsDoc.clear();
    connectionsRootElement = connectionsDoc.createElement("Connections");
    connectionsDoc.appendChild(connectionsRootElement);
    for (const auto &key: devicesMap.keys()) {
        QString deviceName = key;
        DeviceInfo info = devicesMap.value(key);
        QString connType = info.connType;
        QString transferProtocol = info.protocol;
        QString deviceType = info.deviceType;
        if (connType == "Serial"){
            QString port =              (info.serialInfo.port);
            QString baudrate =          (QString::number(info.serialInfo.baudrate));
            QString dataBits =          (QString::number(info.serialInfo.dataBits));
            QString parity =            (info.serialInfo.parity);
            QString stopBits =          (QString::number(info.serialInfo.stopBits));
            QString isTranslating =     (QString::number(info.serialInfo.isTranslating));
            QString TCPCount =          (QString::number(info.serialInfo.tcpCount));
            QString TCPPort =           (QString::number(info.serialInfo.tcpPort));

            QDomElement deviceXml = connectionsDoc.createElement(deviceName.replace(' ','_'));
            connectionsRootElement.appendChild(deviceXml);

            QDomElement protocolXml = connectionsDoc.createElement(connType.replace(' ','_'));
            protocolXml.setAttribute("device_type", deviceType);
            protocolXml.setAttribute("transfer_protocol", transferProtocol);
            protocolXml.setAttribute("Serial_port", port);
            protocolXml.setAttribute("baudrate", baudrate);
            protocolXml.setAttribute("data_bits", dataBits);
            protocolXml.setAttribute("TCP_port_number", TCPPort);
            protocolXml.setAttribute("parity", parity);
            protocolXml.setAttribute("is_translating", isTranslating);
            protocolXml.setAttribute("stop_bits", stopBits);
            protocolXml.setAttribute("TCP_connections_number", TCPCount);
            deviceXml.appendChild(protocolXml);
        }
        else if (connType == "CAN"){
            QString CANType =           (QString::number(info.canInfo.baudrate));
            QString baudrate =          (info.canInfo.type);

            QDomElement deviceXml = connectionsDoc.createElement(deviceName.replace(' ','_'));
            connectionsRootElement.appendChild(deviceXml);

            QDomElement protocolXml = connectionsDoc.createElement(connType.replace(' ','_'));
            protocolXml.setAttribute("device_type", deviceType);
            protocolXml.setAttribute("transfer_protocol", transferProtocol);
            protocolXml.setAttribute("baudrate", baudrate);
            protocolXml.setAttribute("CAN_type", CANType);
            deviceXml.appendChild(protocolXml);
        }
        else if (connType == "TCP"){
            QString clientServer =      (info.tcpInfo.clientServer);
            QString port =              (QString::number(info.tcpInfo.port));
            QString adress =            (info.tcpInfo.adress);

            QDomElement deviceXml = connectionsDoc.createElement(deviceName.replace(' ','_'));
            connectionsRootElement.appendChild(deviceXml);

            QDomElement protocolXml = connectionsDoc.createElement(connType.replace(' ','_'));
            protocolXml.setAttribute("device_type", deviceType);
            protocolXml.setAttribute("transfer_protocol", transferProtocol);
            protocolXml.setAttribute("source", clientServer);
            protocolXml.setAttribute("port_number", port);
            protocolXml.setAttribute("adress", adress);
            deviceXml.appendChild(protocolXml);
        }
    }
    QFile connFile = QFile( dir + '/' + "Configurations" + '/' + "connections.xml" );
    if( !connFile.open( QIODevice::WriteOnly | QIODevice::Text | QIODevice::Truncate) )
    {
        // qDebug( "Failed to open file for writing." );
    }
    QTextStream connStream( &connFile );
    connStream.setCodec("UTF-8");
    connStream << connectionsDoc.toString();
    connFile.close();
}

void MainWindow::saveEvents(QString dir){
    if (dir.isEmpty()) dir = experimentDirectory;
    eventDoc.clear();
    eventRootElement = eventDoc.createElement("Events");
    eventDoc.appendChild(eventRootElement);
    for (const auto &key: eventMap.keys()) {
        QString eventName = key;
        EventData event = eventMap.value(key);
        QString name        = event.name;
        QString device      = event.device;
        QString protocol    = event.protocol;
        QString message     = event.message;
        QString messageId   = event.messageId;
        QString fieldName   = event.fieldName;
        QString field       = QString::number(event.field);
        QString fieldType   = event.fieldType;
        QString text        = event.text;
        QString isGreater   = QString::number(event.intTriggers.isGreater);
        QString isLesser    = QString::number(event.intTriggers.isLesser);
        QString isEqual     = QString::number(event.intTriggers.isEqual);
        QString threshhold  = QString::number(event.intTriggers.threshhold);
        QString startBit    = QString::number(event.bitmapTriggers.startBit);
        QString endBit      = QString::number(event.bitmapTriggers.endBit);
        QString bitValue    = QString::number(event.bitmapTriggers.bitValue);
        QString bitIsEqual  = QString::number(event.bitmapTriggers.isEqual);
        QString charValue   = event.charTriggers.charValue;
        QString status      = QString::number(event.status);
        QDomElement eventXml = eventDoc.createElement(eventName.replace(' ','_'));
        eventXml.setAttribute("name", name);
        eventXml.setAttribute("device", device);
        eventXml.setAttribute("protocol", protocol);
        eventXml.setAttribute("message", message);
        eventXml.setAttribute("messageId", messageId);
        eventXml.setAttribute("fieldName", fieldName);
        eventXml.setAttribute("field", field);
        eventXml.setAttribute("fieldType", fieldType);
        eventXml.setAttribute("text", text);
        eventXml.setAttribute("isGreater", isGreater);
        eventXml.setAttribute("isLesser", isLesser);
        eventXml.setAttribute("isEqual", isEqual);
        eventXml.setAttribute("threshhold", threshhold);
        eventXml.setAttribute("startBit", startBit);
        eventXml.setAttribute("endBit", endBit);
        eventXml.setAttribute("bitValue", bitValue);
        eventXml.setAttribute("bitIsEqual", bitIsEqual);
        eventXml.setAttribute("charValue", charValue);
        eventXml.setAttribute("status", status);
        eventRootElement.appendChild(eventXml);
    }
    QFile eventFile = QFile( dir + '/' + "Configurations" + '/' + "events.xml" );
    if( !eventFile.open( QIODevice::WriteOnly | QIODevice::Text | QIODevice::Truncate) )
    {
        // qDebug( "Failed to open file for writing." );
    }
    QTextStream eventStream( &eventFile );
    eventStream.setCodec("UTF-8");
    eventStream << eventDoc.toString();
    eventFile.close();
}

void MainWindow::saveNotes(QString dir){
    if (dir.isEmpty()) dir = experimentDirectory;
    notesDialog nD(experimentDirectory,isLap,this);
    nD.changeDir(dir);
    nD.saveNotes();
}

void MainWindow::saveExperimentConfiguration(QString dir){
    if (dir.isEmpty()) dir = experimentDirectory;
    experimentConfigurationDialog experimentDialog = experimentConfigurationDialog(experimentDirectory,this);
    experimentDialog.changeDir(dir);
    experimentDialog.saveAll();
}

void MainWindow::saveExperiment(QString dir)
{
    if (dir.isEmpty()) dir = experimentDirectory;
    QDir experimentDir(dir);
    experimentDir.mkpath(dir + '/' + "Notes");
    experimentDir.mkpath(dir + '/' + "Configurations");
    experimentDir.mkpath(dir + '/' + "Configurations" + '/' + "Experiment_configurations");
    experimentDir.mkpath(dir + '/' + "Configurations" + '/' + "Device_configurations");
    experimentDir.mkpath(dir + '/' + "Configurations" + '/' + "Lap_presets");
    saveConnections(dir);
    saveEvents(dir);
    saveExperimentConfiguration(dir);
    saveNotes(dir);
}

void MainWindow::performAction(QAction *action)
{
    if (action == ui->actionLoadExperiment){
        if (exploreExperiment()) loadExperiment(experimentDirectory);
    }
    else if (action == ui->actionSaveExperiment){
        if (experimentDirectory.isEmpty()){
            QString dir = QFileDialog::getExistingDirectory(this, tr("Open Directory"),
                                                            "/home",
                                                            QFileDialog::ShowDirsOnly
                                                                | QFileDialog::DontResolveSymlinks);
            this->experimentDirectory = dir;
        }
        saveExperiment();
    }
    else if (action == ui->actionSaveAsExperiment){
        QString dir = QFileDialog::getExistingDirectory(this, tr("Open Directory"),
                                                        experimentDirectory,
                                                        QFileDialog::ShowDirsOnly
                                                            | QFileDialog::DontResolveSymlinks);
        saveExperiment(dir);
        this->experimentDirectory = dir;
        changeExperimentDirectoryLabel();
    }
    else if (action == ui->actionExperimentConfig){
        if (experimentDirectory.isEmpty()){
            QMessageBox::warning(this, "Ошибка", "Не выбрана директория эксперимента!");
            return;
        }
        experimentConfigurationDialog experimentDialog = experimentConfigurationDialog(experimentDirectory,this);
        if (experimentDialog.exec() == QDialog::Accepted){
            experimentDialog.saveAll();
        }
    }
    else if (action == ui->actionDeviceConfig){
        deviceConfigurationsDialog experimentDialog = deviceConfigurationsDialog(experimentDirectory, devicesMap, connectionsMap, messagesMap, this);
        experimentDialog.exec();
    }
    else if (action == ui->actionSavePreset){
        presetSettingsDialog presetDialog(this);
        if (presetDialog.exec() == QDialog::Accepted){
            Preset preset = presetDialog.getPreset();
            currentPreset = preset;
            updatePreset();
            savePreset();
        }
    }
    else if (action == ui->actionLoadPreset){
        QString dir = QFileDialog::getExistingDirectory(this, tr("Open Directory"),
                                                        experimentDirectory,
                                                        QFileDialog::ShowDirsOnly
                                                            | QFileDialog::DontResolveSymlinks);
        loadPreset(dir);
    }
}

void MainWindow::deleteConnection()
{
    if (ui->tableWidgetConnections->selectedItems().isEmpty()){
        QMessageBox::warning(this, "Ошибка", "Устройство не выбрано!");
        return;
    }
    QTableWidgetSelectionRange range = ui->tableWidgetConnections->selectedRanges().at(0);
    int startRow = range.topRow();
    int endRow = range.bottomRow();
    for (int row = endRow; row >= startRow; row--) {
        QString deviceName = ui->tableWidgetConnections->item(row, 0)->text();
        if (connectionsMap.contains(deviceName)){
            QMessageBox::warning(this, "Ошибка", "Отключите устройство перед удалением!");
            return;
        }
        tableFieldsMap.remove(deviceName);
        foreach (auto key, tableFieldsMap.keys()) {
            TableConnectionsFields *fields = &tableFieldsMap[key];
            if (fields->row > row) fields->row -= 1;
        }
        if (devicesMap[deviceName].connType == "Serial" && devicesMap[deviceName].serialInfo.isTranslating) bridgeMap.remove(deviceName);
        devicesMap.remove(deviceName);
        bufferMap.remove(deviceName);
        newDataMap.remove(deviceName);
        ui->tableWidgetConnections->removeRow(row);
    }
    setupTableSize(ui->tableWidgetConnections);
}

void MainWindow::editConnection()
{
    if (ui->tableWidgetConnections->selectedItems().isEmpty()){
        QMessageBox::warning(this, "Ошибка", "Устройство не выбрано!");
        return;
    }

    QString deviceName = ui->tableWidgetConnections->item(ui->tableWidgetConnections->currentRow(), 0)->text();
    if (connectionsMap.contains(deviceName)){
        QMessageBox::warning(this, "Ошибка", "Отключите устройство перед изменением!");
        return;
    }

    DeviceInfo deviceInfo = devicesMap[deviceName];
    ConnectionSettings cs(deviceInfo, devicesMap, this);
    if (cs.exec() == QDialog::Accepted){
        DeviceInfo info = cs.getSettings();
        QString deviceName = info.ID;
        devicesMap[deviceName] = info;
    }
    fillConnectionsTable();
}


void MainWindow::openConnectionSettings()
{
    ConnectionSettings cs(devicesMap,this);
    if (cs.exec() == QDialog::Accepted){
        DeviceInfo info = cs.getSettings();
        QString deviceName = info.ID;
        if(devicesMap.contains(deviceName)){
            QMessageBox::warning(this, "Ошибка", "Устройство с таким ID уже добавлено!\nУдалите или измените имеющееся если хотите добавить это");
            return;
        }
        devicesMap.insert(deviceName,info);
        addItemToConnectionsTable(info);
    }
}

void MainWindow::openNotes()
{
    notesDialog nD(experimentDirectory,isLap,this);
    nD.exec();
    notesList = nD.getNotes();
}

void MainWindow::openEventSettings()
{
    eventSettingsDialog eSD = eventSettingsDialog(&eventMap, devicesMap, messagesMap, this);
    eSD.exec();
}

/// TODO: надо что-то делать
void MainWindow::openStartStopActions()
{
    QList<QString> devices = devicesMap.keys();
    startStopActionsDialog SSAD = startStopActionsDialog(devices, this);
    if (SSAD.exec() == QDialog::Accepted){
        startStopActionsList = SSAD.getActions();
    }
}

void MainWindow::openDataAndGraphs()
{
    dataAndGraphsDialog oDaGD = dataAndGraphsDialog(devicesMap, connectionsMap, this);
    oDaGD.exec();
}

void MainWindow::connectDevice()
{

    if (ui->tableWidgetConnections->selectedItems().isEmpty()){
        QMessageBox::warning(this, "Ошибка", "Устройство не выбрано!");
        return;
    }

    int row = ui->tableWidgetConnections->currentRow();
    QString deviceName = ui->tableWidgetConnections->item(row,INDEX_CONN_TABLE_ID)->text();
    if (connectionsMap.contains(deviceName)){
        QMessageBox::warning(this, "Ошибка", "Это устройство уже подключено!");
        return;
    }
    QString protocolName = ui->tableWidgetConnections->item(row,INDEX_CONN_TABLE_TYPE)->text();
    QTableWidgetItem *onnOffItem = tableFieldsMap[deviceName].onOff;
    onnOffItem->setBackground(QBrush(QColor(255,255,0)));
    if (protocolName == "Serial"){
        QString port;
        int baudrate;
        int dataBits;
        int stopBits;
        QSerialPort* connection = new QSerialPort(this);
        QSerialPort::Parity parity = QSerialPort::NoParity;
        QString parityStr = devicesMap[deviceName].serialInfo.parity;
        if (parityStr == "Нет"){
            parity = QSerialPort::NoParity;
        }
        if (parityStr == "Четное"){
            parity = QSerialPort::EvenParity;
        }
        if (parityStr == "Нечетное"){
            parity = QSerialPort::OddParity;
        }
        dataBits = devicesMap[deviceName].serialInfo.dataBits;
        QSerialPort::DataBits databitsEnum;
        switch (dataBits) {
        case 5:
            databitsEnum = QSerialPort::Data5;
            break;
        case 6:
            databitsEnum = QSerialPort::Data6;
            break;
        case 7:
            databitsEnum = QSerialPort::Data7;
            break;
        case 8:
            databitsEnum = QSerialPort::Data8;
            break;
        default:
            databitsEnum = QSerialPort::Data8;
            break;
        }
        stopBits = devicesMap[deviceName].serialInfo.stopBits;
        QSerialPort::StopBits stopBitsEnum;
        switch (stopBits) {
        case 1:
            stopBitsEnum = QSerialPort::OneStop;
            break;
        case 2:
            stopBitsEnum = QSerialPort::TwoStop;
            break;
        default:
            stopBitsEnum = QSerialPort::OneStop;
            break;
        }
        connection->setDataBits(databitsEnum);
        connection->setStopBits(stopBitsEnum);
        connection->setParity(parity);
        port = devicesMap[deviceName].serialInfo.port;
        baudrate = devicesMap[deviceName].serialInfo.baudrate;
        connection->setPortName(port);
        connection->setBaudRate(baudrate);
        // пробуем подключится
        if (!connection->open(QIODevice::ReadWrite)) {
            onnOffItem->setBackground(QBrush(QColor(255,0,0)));
            QMessageBox::warning(this, "Ошибка", "Не удалось подключится к порту");
            return;
        }
        connect(connection, &QSerialPort::readyRead, this, [this,deviceName]() {this->readDevice(deviceName);});
        onnOffItem->setBackground(QBrush(QColor(0,255,0)));
        connectionsMap.insert(deviceName, connection);
        DeviceInfo info = devicesMap[deviceName];
        if (info.connType == "Serial" && info.serialInfo.isTranslating){
            SerialToTcpBridge *bridge = new SerialToTcpBridge(info.serialInfo.tcpCount, info.serialInfo.tcpPort, this);
            bridgeMap.insert(deviceName,bridge);
            connect(bridge, SIGNAL(newConnectionCount()), this, SLOT(onNewBridgeConnection()));
            bridge->start();
        }
        QByteArray* buffer = new QByteArray();
        bufferMap.insert(deviceName, buffer);
    }
    else if (protocolName == "TCP"){
        QTcpSocket* connection = new QTcpSocket(this);
        QString port = QString::number(devicesMap[deviceName].tcpInfo.port);
        QString address = devicesMap[deviceName].tcpInfo.adress;
        // Подключаем сигналы
        connect(connection, &QTcpSocket::connected, this, [connection, onnOffItem, this, row, deviceName]() {
            onnOffItem->setBackground(QBrush(QColor(0,255,0)));
            this->connectionsMap.insert(deviceName, connection);
            QByteArray* buffer = new QByteArray();
            this->bufferMap.insert(deviceName, buffer);
            connect(connection, &QTcpSocket::readyRead, this, [this, deviceName]() {this->readDevice(deviceName);});
        });

        connect(connection, &QTcpSocket::disconnected, this, [connection, onnOffItem, this, row]() {
            onnOffItem->setBackground(QBrush(QColor(255,0,0)));
            return;
        });

        // Запускаем асинхронное подключение
        connection->connectToHost(address, port.toInt());
    }
}

void MainWindow::disconnectDevice()
{
    if (ui->tableWidgetConnections->selectedItems().isEmpty()){
        QMessageBox::warning(this, "Ошибка", "Устройство не выбрано!");
        return;
    }

    int row = ui->tableWidgetConnections->currentRow();
    QString deviceName = ui->tableWidgetConnections->item(row,INDEX_CONN_TABLE_ID)->text();
    QTableWidgetItem *onnOffItem = tableFieldsMap[deviceName].onOff;
    if (!connectionsMap.contains(deviceName)){
        QMessageBox::warning(this, "Ошибка", "Это устройство не подключено!");
        onnOffItem->setBackground(QBrush(QColor(255,0,0)));
        connectionsMap.remove(ui->tableWidgetConnections->item(row,INDEX_CONN_TABLE_ID)->text());
        return;
    }
    QObject* connection = connectionsMap[deviceName];
    if (qobject_cast<QIODevice*>(connection)){
        QIODevice* ioCon = qobject_cast<QIODevice*>(connection);
        ioCon->close();
    }
    onnOffItem->setBackground(QBrush(QColor(255,0,0)));
    delete bufferMap[deviceName];
    if (devicesMap[deviceName].connType == "Serial" && devicesMap[deviceName].serialInfo.isTranslating){
        SerialToTcpBridge* bridge = bridgeMap[deviceName];
        bridge->stop();
        bridgeMap.remove(deviceName);
    }
    bufferMap.remove(deviceName);
    connectionsMap.remove(deviceName);
}

void MainWindow::sendUserEvent()
{
    if(!isLap) return;
    QString localTime = QTime::currentTime().toString("hh:mm:ss.zzz");
    QString event = ui->lineEditAddEvent->text();
    addItemToLogTable(localTime, "", event);
    ui->lineEditAddEvent->clear();
}

void MainWindow::clearLogTable()
{
    ui->tableWidgetLog->clearContents();
    ui->tableWidgetLog->setRowCount(0);
}

void MainWindow::addItemToLogTable(QString localTime, QString GNSSTime, QString event)
{
    QScrollBar* scrollBar = ui->tableWidgetLog->verticalScrollBar();
    bool isAtBottom = scrollBar->value() == scrollBar->maximum();
    int row = ui->tableWidgetLog->rowCount()+1;
    ui->tableWidgetLog->setRowCount(row);
    QTableWidgetItem* localTimeItem = new QTableWidgetItem(localTime);
    QTableWidgetItem* GNSSTimeItem = new QTableWidgetItem(GNSSTime);
    QTableWidgetItem* eventItem = new QTableWidgetItem(event);
    ui->tableWidgetLog->setItem(row-1,0,localTimeItem);
    ui->tableWidgetLog->setItem(row-1,1,GNSSTimeItem);
    ui->tableWidgetLog->setItem(row-1,2,eventItem);
    if (isAtBottom) ui->tableWidgetLog->scrollToBottom();
}

QString MainWindow::tableWidgetToString(QTableWidget* table)
{
    if (!table) return QString();

    QString result;
    QTextStream stream(&result);

    // Получаем размеры таблицы
    int rows = table->rowCount();
    int cols = table->columnCount();

    if (rows == 0 || cols == 0) return QString();

    // Определяем максимальную ширину каждого столбца
    QVector<int> columnWidths(cols, 0);

    // Учитываем заголовки столбцов
    for (int col = 0; col < cols; ++col) {
        QString headerText = table->horizontalHeaderItem(col) ?
                                 table->horizontalHeaderItem(col)->text() :
                                 QString("Column %1").arg(col + 1);
        columnWidths[col] = qMax(columnWidths[col], headerText.length());
    }

    // Учитываем содержимое ячеек
    for (int row = 0; row < rows; ++row) {
        for (int col = 0; col < cols; ++col) {
            QTableWidgetItem* item = table->item(row, col);
            if (item) {
                QString cellText = item->text();
                columnWidths[col] = qMax(columnWidths[col], cellText.length());
            }
        }
    }

    // Добавляем небольшой отступ (2 пробела с каждой стороны)
    for (int col = 0; col < cols; ++col) {
        columnWidths[col] += 4;
    }

    // Функция для создания разделительной линии
    auto addSeparator = [&]() {
        for (int col = 0; col < cols; ++col) {
            stream << "+" << QString(columnWidths[col], '-');
        }
        stream << "+" << Qt::endl;
    };

    // Функция для добавления строки с выравниванием
    auto addRow = [&](const QStringList& texts, bool isHeader = false) {
        stream << "|";
        for (int col = 0; col < cols; ++col) {
            QString text = (col < texts.size()) ? texts[col] : "";
            int padding = columnWidths[col] - text.length();
            int leftPadding = padding / 2;
            int rightPadding = padding - leftPadding;

            if (isHeader) {
                // Заголовки выделяем жирным (если нужно, можно добавить символы)
                stream << QString(leftPadding, ' ') << text << QString(rightPadding, ' ') << "|";
            } else {
                stream << QString(leftPadding, ' ') << text << QString(rightPadding, ' ') << "|";
            }
        }
        stream << Qt::endl;
    };

    // Верхняя граница
    addSeparator();

    // Заголовки столбцов
    QStringList headers;
    for (int col = 0; col < cols; ++col) {
        headers << (table->horizontalHeaderItem(col) ?
                        table->horizontalHeaderItem(col)->text() :
                        QString("Col %1").arg(col + 1));
    }
    addRow(headers, true);

    // Разделитель после заголовков
    addSeparator();

    // Данные таблицы
    for (int row = 0; row < rows; ++row) {
        QStringList rowData;
        for (int col = 0; col < cols; ++col) {
            QTableWidgetItem* item = table->item(row, col);
            rowData << (item ? item->text() : "");
        }
        addRow(rowData);
    }

    // Нижняя граница
    addSeparator();

    return result;
}
/// TODO: Сравнение пресета и текущих настроек
/// При нажатии "Старт" при отсутствии пресета попросить выбрать пресет
/// При отличиях от текущего попросить сохранить
/// Нужно как-то проверять отличия
/// Для Mounting и Connections сравнивать мапы, отличия записывать в строку отличий и выводить
/// Шаблон:
/// "Замечены отличия выбранного пресета и текущих настроек
/// Отличия:
///     Пресет                      Текущие настройки
/// Отсутсвует устройство _             Устройство _ присутствует
/// Настройки _:                        Настройки _:
/// Отсутствует подключение _ с _       подключение _ с _ присутствует
/// Настройки подключения _ с _:        Настройки подключения _ с _:
/// Отсутсвуют настройки установки _:   настройки установки присутствуют
/// Настройки установки _:              Настройки установки _:"
///
///
void MainWindow::checkPreset(){
    // QString currentDirectory = experimentDirectory + '/' + "Configurations" + '/' + "Experiment_configurations";
    // QString experimentConnectionsDirectory = currentDirectory + '/' + "Experiment_connections.yaml";
    // QString experimentMountingDirectory = currentDirectory + '/' + "Experiment_mounting.yaml";
    // auto currentDevicesMap = experimentConfigurationDialog::getDevicesFromFile(experimentConnectionsDirectory);
    // auto currentConnectionsMap = experimentConfigurationDialog::getConnectionsFromFile(experimentConnectionsDirectory);
    // auto currentMountingDirectory = experimentConfigurationDialog::getMountingFromFile(experimentMountingDirectory);
    // auto presetDevicesMap = currentPreset.devicesMap;
    // auto presetConnectionsMap = currentPreset.connectionsMap;
    // auto presetMountingDirectory = currentPreset.mountingMap;
    // QString totalDiff;
    // /// Проверка наличия мапы
    // /// Проверка наличия элемента в мапе (Сбор ключей с двух мап, проход по этим ключам и вывод сообщения о разнице)
    // QStringList devicesKeys;
    // devicesKeys.append(currentDevicesMap.keys());
    // devicesKeys.append(presetDevicesMap.keys());
    // QStringList devicesUniqueKeys;
    // foreach (QString key, devicesKeys) {
    //     if (!devicesUniqueKeys.contains(key)) devicesUniqueKeys.append(key);
    // }
    // foreach (QString key, devicesUniqueKeys) {
    //     QString diffLine = (!currentDevicesMap.contains(key))? QString("Отсутствует устройство %1\t\tУстройство %1 присутствует").arg(key):
    //         (!presetDevicesMap.contains(key))? QString("Устройство %1 присутствует\t\tОтсутствует устройство %1").arg(key): "";
    //     if (!diffLine.isEmpty()) totalDiff.append(diffLine + "\n");
    // }
    // QStringList connectionsKeys;
    // connectionsKeys.append(currentConnectionsMap.keys());
    // connectionsKeys.append(presetConnectionsMap.keys());
    // QStringList connectionsUniqueKeys;
    // foreach (QString key, connectionsKeys) {
    //     if (!connectionsUniqueKeys.contains(key)) connectionsUniqueKeys.append(key);
    // }
    // foreach (QString key, connectionsUniqueKeys) {
    //     QString diffLine = (!currentConnectionsMap.contains(key))? diffLine = QString("Отсутствует подключение %1\t\tПодключение %1 присутствует").arg(key):
    //        (!presetConnectionsMap.contains(key))? diffLine = QString("Подключение %1 присутствует\t\tОтсутствует подключение %1").arg(key):"";
    //     if (!diffLine.isEmpty()) totalDiff.append(diffLine + "\n");
    // }
    // if (!totalDiff.isEmpty()){
    //     QMessageBox::warning(this,"Выбранный пресет отличается от текущих настроек!",totalDiff);
    // }
}

void MainWindow::processStartStopActions(bool isStart){
    foreach (auto action, startStopActionsList) {
        if (action.isStart != isStart) continue;
        QString dir = action.fileDir;
        QString deviceName = action.deviceName;
        if (!connectionsMap[deviceName]) continue;
        QObject* connection = connectionsMap[deviceName];
        QIODevice* ioCon = qobject_cast<QIODevice*>(connection);
        if (!ioCon->isOpen()) continue;
        DeviceInfo info = devicesMap[deviceName];
        QString protocol = info.protocol;
        QFile file(dir);
        if (!file.open(QFile::ReadOnly)) continue;
        if (protocol == "Ublox"){
            QString line;
            QTextStream stream(&file);
            while(!stream.atEnd()){
                line = stream.readLine();
                QStringList hexBytes = line.split(' ', Qt::SkipEmptyParts);

                QByteArray byteArray;
                for (const QString& hexByte : hexBytes) {
                    bool ok;
                    quint8 byte = hexByte.toUInt(&ok, 16);
                    if (ok) {
                        byteArray.append(byte);
                    } else {
                        qDebug() << "Ошибка преобразования:" << hexByte;
                    }
                }
                UbloxParser parser(connection);
                // qDebug() << msg.toHex(' ');
                parser.sendMessage(byteArray);
            }
        }
        else if (protocol == "Unicore"){
            QString line;
            QTextStream stream(&file);
            while(!stream.atEnd()){
                line = stream.readLine();
                UnicoreParser parser(connection);
                parser.sendMessage(line);
            }
        }
    }
}

void MainWindow::startExperiment()
{
    if (!isLap){
        checkPreset();
    }
    isLap = !isLap;
    processStartStopActions(isLap);
    if (isLap){

        lapNumber++;
        lapTime = QTime::currentTime();
        ui->pushButtonStart->setStyleSheet("image: url(:/resources/stop.png);\n"
                                           " background-color: qlineargradient(spread:pad, x1:1, y1:1, x2:1, y2:1, stop:1 rgba(0, 0, 0, 0));\n"
                                           " border-color: qlineargradient(spread:pad, x1:1, y1:1, x2:1, y2:1, stop:1 rgba(0, 0, 0, 0));\n"
                                           " border-radius: 150px;");
        QString localTime = QTime::currentTime().toString("hh:mm:ss.zzz");
        QString event = "Эксперимент начат";
        addItemToLogTable(localTime, "", event);

        foreach (QString connDevice, connectionsMap.keys()) {
            QObject* connection = connectionsMap[connDevice];
            if (qobject_cast<QIODevice*>(connection)){
                QIODevice* ioCon = qobject_cast<QIODevice*>(connection);
                if (!ioCon->isOpen()) continue;
            }
            foreach (QString eventName, eventMap.keys()) {
                EventData* event = &eventMap[eventName];
                event->status = INDEX_FLAGS_UNKNOWN;
                QString eventDevice = event->device;
                if (connDevice != eventDevice) continue;
                if (event->protocol == "Ublox"){
                    uint8_t messClass = messagesMap[event->message].id.left(2).toUInt(nullptr,16);
                    uint8_t messId = messagesMap[event->message].id.right(2).toUInt(nullptr,16);
                    U1 hdrdata[] = {0xb5, 0x62};
                    QByteArray hdr(reinterpret_cast<char*>(hdrdata), sizeof(hdrdata));
                    U1 endata[] = {CFG::MSG::classID, CFG::MSG::messageID, 0x08, 0x00};
                    QByteArray en(reinterpret_cast<char*>(endata), sizeof(endata));
                    U1 payloaddata[] = {messClass, messId, 0x01, 0x01, 0x01, 0x01, 0x01, 0x00};
                    QByteArray payload(reinterpret_cast<char*>(payloaddata), sizeof(payloaddata));
                    QByteArray msg = hdr + en + payload;
                    QByteArray checkSum = UbloxParser::calcCheckSum(en + payload);
                    msg.append(checkSum);
                    UbloxParser parser(connection);
                    // qDebug() << msg.toHex(' ');
                    parser.sendMessage(msg);
                }
                else if (event->protocol == "Unicore"){
                    UnicoreParser parser(connection);
                    // qDebug() << event->message + "B 1";
                    parser.sendMessage(event->message + "B 1");
                }
            }
        }
    }
    else{
        ui->pushButtonStart->setStyleSheet("image: url(:/resources/start.png);\n"
                                           " background-color: qlineargradient(spread:pad, x1:1, y1:1, x2:1, y2:1, stop:1 rgba(0, 0, 0, 0));\n"
                                           " border-color: qlineargradient(spread:pad, x1:1, y1:1, x2:1, y2:1, stop:1 rgba(0, 0, 0, 0));\n"
                                           " border-radius: 150px;");
        QString localTime = QTime::currentTime().toString("hh:mm:ss.zzz");
        QString event = "Эксперимент завершен";
        addItemToLogTable(localTime, "", event);
        QDir experimentDir(experimentDirectory);
        QString lapDir = QString(experimentDirectory + "/Lap_%1").arg(lapNumber);
        experimentDir.mkpath(lapDir);
        QString eventLogDir = lapDir + "/Event_log.txt";
        QFile eventLogFile(eventLogDir);
        eventLogFile.open(QIODevice::ReadWrite);
        QString tableContents = tableWidgetToString(ui->tableWidgetLog);
        QTextStream eventLogStream(&eventLogFile);
        eventLogStream.setCodec("UTF-8");
        eventLogStream << tableContents;
        eventLogFile.close();

        QString infoDir = lapDir + "/Info.txt";
        QFile infoFile(infoDir);
        infoFile.open(QIODevice::ReadWrite);
        QTextStream infoStream(&infoFile);
        infoStream.setCodec("UTF-8");
        infoStream << QString("Пресет: %1").arg(currentPreset.title);
        if (!currentPreset.description.isEmpty()){
            infoStream << QString("\nОписание пресета: %1").arg(currentPreset.description);
        }
        infoFile.close();
    }
}

void MainWindow::indicateData()
{
    foreach (QString device, tableFieldsMap.keys()) {
        TableConnectionsFields *fields = &tableFieldsMap[device];
        QTableWidgetItem* dataItem = fields->data;
        if (fields->dataTimer == 0) {
            dataItem->setBackground(QBrush(QColor(0,100,0)));
            continue;
        }
        fields->dataTimer -= 1;
        dataItem->setBackground(QBrush(QColor(0,255,0)));
    }
}

void MainWindow::readDevice(QString deviceName)
{
    QObject* connection = connectionsMap[deviceName];
    QByteArray buff;
    if (connection == nullptr) return;
    if (qobject_cast<QIODevice*>(connection)){
        QIODevice* ioCon = qobject_cast<QIODevice*>(connection);
        if (!ioCon->isOpen()) return;

        buff.append(ioCon->readAll());
        tableFieldsMap[deviceName].dataTimer = 100;
    }
    if (buff.isEmpty()) return;
    NewData data;
    data.deviceInfo = devicesMap[deviceName];
    data.buff = buff;
    newDataMap[deviceName] = data;
    if (data.deviceInfo.connType == "Serial" && data.deviceInfo.serialInfo.isTranslating){
        SerialToTcpBridge* bridge = bridgeMap[deviceName];
        bridge->write(buff);
    }
    bufferMap[deviceName]->append(buff);
    parseMessage(data);
    emit newData(data);
}

QMap<QString,NewData> MainWindow::getNewData(){
    return newDataMap;
}

void MainWindow::changeLapTimer(){
    if (!isLap) return;
    QTime currTime = QTime::currentTime();
    int currMsec = currTime.msec() + (currTime.second() + currTime.minute()*60 + currTime.hour()*3600)*1000;
    int startMsec = lapTime.msec() + (lapTime.second() + lapTime.minute()*60 + lapTime.hour()*3600)*1000;
    int diffMsec = currMsec - startMsec;
    ui->labelElapsedTime->setText(QString("%1.%2").arg(QString::number(diffMsec/1000),QString::number(diffMsec%1000).rightJustified(3,'0')));
}

void MainWindow::parseMessage(NewData data)
{
    QString connDevice = data.deviceInfo.ID;
    if (!isLap) {
        QByteArray *buff = bufferMap[connDevice];
        if (buff) buff->clear();
        return;
    }
    // ui->labelElapsedTime->setText(QString("%1.%2").arg(diffSeconds,diffMsec)); //прикол
    QObject* connection = connectionsMap[connDevice];
    QByteArray *buff = bufferMap[connDevice];
    if (connection == nullptr) return;
    if (buff->isEmpty()) return;
    while(!buff->isEmpty()){
        QByteArray messData;
        QString messId;
        QString GNSSTime;
        QDataStream::ByteOrder order;
        DeviceInfo deviceInfo = devicesMap[connDevice];
        QString protocol = deviceInfo.protocol;
        if (protocol == "Ublox"){
            UbloxParser parser(connection);
            UbloxMessage mess = parser.parseMessage(buff);
            if (mess.data.isEmpty()) return;
            messData = mess.data;
            messId = QString::number(mess.messClass, 16).rightJustified(2,'0') + QString::number(mess.messId, 16).rightJustified(2,'0');
            order = QDataStream::LittleEndian;
        }
        else if (protocol == "Unicore"){
            UnicoreParser parser(connection);
            UnicoreMessage mess = parser.parseBinaryMessage(buff);
            if (mess.data.isEmpty()) return;
            messData = mess.data;
            messId = QString::number(mess.binaryHeader.messageId);
            GNSSTime = QString::number(mess.binaryHeader.ms);
            order = QDataStream::BigEndian;
        }
        else return;
        qDebug() << "------------------\n";
        foreach (QString eventName, eventMap.keys()) {
            EventData *event = &eventMap[eventName];
            qDebug() << eventName;
            if (event->device != connDevice) continue;
            if (event->messageId != messId) continue;
            Mess::Field field = messagesMap[event->message].fields[event->fieldName];
            int offset = field.offset;
            int size = field.size;
            double scale = field.scale;
            QByteArray data = messData.mid(offset,size);
            QDataStream stream(data);
            stream.setByteOrder(order);
            bool check = false;
            // qDebug() << '\n';
            // qDebug() << ("event: " + event->name) << ("field: " + event->fieldName) << ("type: " + event->fieldType);
            if (event->fieldType == "int" || event->fieldType == "uint" || event->fieldType == "float" || event->fieldType == "double"){
                int flags = (int)event->intTriggers.isEqual +
                            ((int)event->intTriggers.isGreater << 1) +
                            ((int)event->intTriggers.isLesser << 2);
                double thresh = event->intTriggers.threshhold / scale;
                if (event->fieldType == "int" && size == 1) check = compareValue<qint8>(stream, thresh, flags);
                else if (event->fieldType == "uint" && size == 1) check = compareValue<quint8>(stream, thresh, flags);
                else if (event->fieldType == "int" && size == 2) check = compareValue<qint16>(stream, thresh, flags);
                else if (event->fieldType == "uint" && size == 2) check = compareValue<quint16>(stream, thresh, flags);
                else if (event->fieldType == "int" && size == 4) check = compareValue<qint32>(stream, thresh, flags);
                else if (event->fieldType == "uint" && size == 4) check = compareValue<quint32>(stream, thresh, flags);
                else if (event->fieldType == "int" && size == 8) check = compareValue<qint64>(stream, thresh, flags);
                else if (event->fieldType == "uint" && size == 8) check = compareValue<quint64>(stream, thresh, flags);
                else if (event->fieldType == "float") check = compareValue<float>(stream, thresh, flags);
                else if (event->fieldType == "double") check = compareValue<double>(stream, thresh, flags);
                else if (event->fieldType == "int") check = compareValue<qint32>(stream, thresh, flags);
                else if (event->fieldType == "uint") check = compareValue<quint32>(stream, thresh, flags);
                else {
                    qWarning() << "Unsupported field type:" << event->fieldType;
                }
            }
            else if (event->fieldType == "char"){
                // qDebug() << ("data: " + QString::fromLatin1(data)) << ("event value: " + event->charTriggers.charValue);
                check = QString::fromLatin1(data) == event->charTriggers.charValue;
            }
            else if (event->fieldType == "bitmap"){
                if (size == 1) check = compareBitmap<uint8_t>(stream, event->bitmapTriggers.startBit, event->bitmapTriggers.endBit, event->bitmapTriggers.bitValue);
                else if (size == 2) check = compareBitmap<uint16_t>(stream, event->bitmapTriggers.startBit, event->bitmapTriggers.endBit, event->bitmapTriggers.bitValue);
                else if (size == 4) check = compareBitmap<uint32_t>(stream, event->bitmapTriggers.startBit, event->bitmapTriggers.endBit, event->bitmapTriggers.bitValue);
                else {
                    qWarning() << "Unsupported size:" << event->fieldType;
                }
                if (!event->bitmapTriggers.isEqual) check = !check;
            }
            if (check){
                if (event->status != INDEX_FLAGS_TRUE){
                    QString localTime = QTime::currentTime().toString("hh:mm:ss.zzz");
                    event->status = INDEX_FLAGS_TRUE;
                    if (protocol == "Ublox"){
                        int offset = messagesMap[event->message].fields["itow"].offset;
                        int size = messagesMap[event->message].fields["itow"].size;
                        double scale = messagesMap[event->message].fields[event->fieldName].scale;
                        QByteArray data = messData.mid(offset,size);
                        QDataStream stream(data);
                        stream.setByteOrder(order);
                        uint32_t itow;
                        stream >> itow;
                        GNSSTime = QString::number((double)itow*scale,'f',0);
                    }
                    addItemToLogTable(localTime, GNSSTime, event->text);
                }
            }
            else{
                event->status = INDEX_FLAGS_FALSE;
            }
        }
    }
}


