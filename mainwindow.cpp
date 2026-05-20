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


/// TODO: разветвление потока
/// Задача:
/// Нужно добавить возможность ретранслировать данные из подключенного Serial потра
/// на несколько TCP соединений без влияния на работу приложения.
/// Идея:
/// С Serial порта мы читаем ТОЛЬКО в главном окне, сохраняем данные в контейнер newData
/// Далее этот контейнер передается нуждающимся объектам и сущностям
/// Как реализовывать:
/// Добавляем слот readPorts и массив структур newData содержащая инфу об источнике и QByteArray newData  для MainWindow.
/// В нем читаются порты и новая информация добавляется в мапу буферов,
/// отправляется сигнал newData, также при получении новой инфы загораются
/// на определенное время ячейки данных в таблице.
/// В конфигурации устройств и других окнах добавляется ссылка на MainWindow, поле buff
/// а также слот getNewData, привязанный к сигналу newData, в котором читается новая структура,
/// добавляющий инфу в buf то есть что-то на подобие: connect(mainWindow, SIGNAL(newData), this, SLOT(getNewData))
/// У tcp моста в функции start остается только адрес сервера,
/// также метод sendData и поле buff которое заполняется от этого метода.
/// onSerialReadyRead отпраляет buff в сокеты и очищается
/// Во всех функциях parseMessage ведется работа с buff.
///
/// План:
/// 1. Передалать проект под задумку выше
/// 2. Добавить в проект TCP мост
/// 3. Протестировать разветвление
///


/// Мысли по поводу принципа работы приложения
/// Чтение с портов:
/// Чтение с портов происходит только в одном месте*, а в остальные окна отправляется уже обработанная информация
/// Читаем порт, запускаем парсеры, парсим сообщения получаем наследников структуры Message, закидываем их в массив
/// При запуске какого либо из окон передаем ему также ссылку на этот массив, с ним же и работаем
///
/// *В конфигурацию устройств пользователь не должен заходить посередине захода, следовательно там можно читать с порта
/// *Во вкладке терминала нужно предупеждать о завершении захода

/// Мысли по поводу правок:
/// 1. Надо реализовать подключение по TCP. Выполнено, мыслей нет
/// 2. Новые события для “Настройка событий”. Пока не представляю как это возможно сделать
/// КАСТОМНЫЕ СОБЫТИЯ
/// я) Наименование события.
/// а) Выбор устройства (на котором мониторим возникновение события).
/// б) Выбор сообщения (список сообщений протокола выбранного устройства). (нужно будет составить список сообщений)
/// в) Номер пар-ра в сообщении. Можно ограничиться названием, а по словарю определять номер
/// нужно составить алгоритм работы событий - единый для всех устройств
/// Должен быть: контейнер содержащий в себе название события, название устройства,
/// индентификатор сообщения, номер параметра, тип проверки, доп параметры для типа проверки
/// При создании нового события нужно добавить в контейнер новый элемент, причем это распространяется и на заготовленные события
/// При запуске эксперимента происходит проход по списку и отправляются сообщения на периодическую отправку заданных сообщений
/// Во время эксперимента парсятся сообщения, а затем отправляются на проверку:
/// В цикле идет сравнение со всеми элементами контейнера, при совпадении происходит унифицированный алгоритм проверки:
/// Сначала смотрится тип проверки, затем по параметрам определяется, произошло событие или нет (ставится флаг),
/// затем выводится сообщение
/// Для нахождения событий полей флагов в Unicore придется работать с двоичным протоколом,
/// а это значит что нужно расшифровывать сообщения и для него под свои нужды



/// При изменении/удалении чего либо имеющего свой файл нужно "на ходу" изменить этот файл !!!! надо уточнить !!!!
/// TODO: Загрузка эксперимента
///
/// TODO: Сохранение эксперимента
///
/// TODO: Разобраться с функционалом окна “Действия при старте/остановке захода”
/// Скорее всего после нажатия на "готово" в папку "Lap_presets" сохраняются пары Устройство - конфиг
/// и при запуске захода подобранные таким образом устройства настраиваются по этим конфигам
///
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
        qDebug() << prefix << it.key() << "->" << it.value();
    }
}

template<typename K, typename InnerK, typename InnerV>
void dumpNestedMap(const QMap<K, QMap<InnerK, InnerV>> &m, int indent = 0)
{
    QString prefix(indent, ' ');
    for (auto it = m.cbegin(); it != m.cend(); ++it) {
        qDebug() << prefix << "Key:" << it.key();
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
                qDebug() << "remove file" << filePath << " failed!";
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
            qDebug() << "remove dir" << directory.path() << " failed!";
            error = true;
        }
    }
    return !error;
}

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    ui->tableWidgetConnections->setEditTriggers(QAbstractItemView::NoEditTriggers);
    QWidgetList allWidgets = QApplication::allWidgets();
    foreach (QWidget* widget, allWidgets) {
        QPushButton* button = qobject_cast<QPushButton*>(widget);
        if (button) {
            button->setEnabled(false);
        }
    }
    ui->actionLoadExperiment->setProperty("root","Эксперимент");
    ui->actionSaveExperiment->setProperty("root", "Эксперимент");
    ui->actionLoadPreset->setProperty("root","Пресет");
    ui->actionSavePreset->setProperty("root","Пресет");

    connectionsRootElement = connectionsDoc.createElement("Connections");
    connectionsDoc.appendChild(connectionsRootElement);

    eventRootElement = eventDoc.createElement("Events");
    eventDoc.appendChild(eventRootElement);

    getMessagesConfig();

    QTimer *timer = new QTimer(this);
    connect(timer, SIGNAL(timeout()), this, SLOT(readPorts()));
    connect(timer, SIGNAL(timeout()), this, SLOT(indicateData()));
    connect(timer, SIGNAL(timeout()), this, SLOT(parseMessage()));
    timer->start(1);


    QLabel* versionLabel = new QLabel(tr("Версия ПО: ") + this->version);
    statusBar()->addWidget(versionLabel);
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
        QString TCPPort = info.serialInfo.tcpPort;
        QString string = QString("%1 (%2 из %3)").arg(TCPPort).arg(0).arg(QString::number(info.serialInfo.tcpCount));
        fields.TCPPort->setText(string);
    }
    else if (connType == "CAN"){
        // ничего
    }
    else if (connType == "TCP"){
        QString port =  info.tcpInfo.port;

        fields.TCPPort->setText(port);
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

void MainWindow::addConnectionFromFile()
{
    QString dir = QFileDialog::getOpenFileName(this,
                                               tr(""), "/home", tr("Connection file (*.xml)"));
    if (dir == "") {return;}

    QFile file(dir);
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
            QString stopBits =          parametersXml.attribute("stop_bits");
            QString tcpCount =          parametersXml.attribute("TCP_connections_number");
            info.serialInfo.port = port;
            info.serialInfo.baudrate = baudrate.toInt();
            info.serialInfo.dataBits = dataBits.toInt();
            info.serialInfo.parity = parity;
            info.serialInfo.stopBits = stopBits.toInt();
            info.serialInfo.tcpCount = tcpCount.toInt();
            info.serialInfo.tcpPort = tcpPort;
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
            info.tcpInfo.port = port;
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
/// TODO:: Можно выбрать пустую папку, если в ней нет нужных файлов, то они создаются пустыми, а походу работы с приложением
/// заполняются
void MainWindow::performAction(QAction *action)
{
    if (action->text() == "Загрузить" && action->property("root") == "Эксперимент" ){
        QString dir = QFileDialog::getExistingDirectory(this, tr("Open Directory"),
                                                        "/home",
                                                        QFileDialog::ShowDirsOnly
                                                            | QFileDialog::DontResolveSymlinks);
        if (dir == "") {return;}
        this->experimentDirectory = dir;
        this->devicesMap.clear();
        this->eventMap.clear();
        ///Чтение конфига устройств и заполнение таблицы

        QString connectionsConfigDir = dir + '/' + "Configurations" + '/' + "connections.xml";
        QFile file(connectionsConfigDir);
        file.open(QIODevice::ReadWrite) /*return*/;
        QDomDocument doc("document");
        file.close();
        if (!doc.setContent(&file)) /*return*/;

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
                QString stopBits =          parametersXml.attribute("stop_bits");
                QString tcpCount =          parametersXml.attribute("TCP_connections_number");
                info.serialInfo.port = port;
                info.serialInfo.baudrate = baudrate.toInt();
                info.serialInfo.dataBits = dataBits.toInt();
                info.serialInfo.parity = parity;
                info.serialInfo.stopBits = stopBits.toInt();
                info.serialInfo.tcpCount = tcpCount.toInt();
                info.serialInfo.tcpPort = tcpPort;
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
                info.tcpInfo.port = port;
                info.tcpInfo.adress = adress;
            }

            if(!devicesMap.contains(deviceName)){
                devicesMap[deviceName] = info;
            }
            deviceXml = deviceXml.nextSibling();
        }
        fillConnectionsTable();
        setupTableSize(ui->tableWidgetConnections);

        QString eventsConfigDir = dir + '/' + "Configurations" + '/' + "events.xml";
        QFile eventFile(eventsConfigDir);
        eventFile.open(QIODevice::ReadWrite) /*return*/;
        QDomDocument eventDoc("document");
        eventFile.close();
        if (!eventDoc.setContent(&eventFile)) /*return*/;

        docElem = eventDoc.documentElement();
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
        notesDialog nD(experimentDirectory,isLap,this);
        notesList = nD.getNotes();
        qDebug() << notesList;
        QWidgetList allWidgets = QApplication::allWidgets();
        foreach (QWidget* widget, allWidgets) {
            QPushButton* button = qobject_cast<QPushButton*>(widget);
            if (button) {
                button->setEnabled(true);
            }
        }
        QDir experimentDir(experimentDirectory);
        experimentDir.mkpath(experimentDirectory + '/' + "Configurations" + '/' + "Experiment_configurations");
        experimentDir.mkpath(experimentDirectory + '/' + "Configurations" + '/' + "Device_configurations");
        experimentDir.mkpath(experimentDirectory + '/' + "Configurations" + '/' + "Lap_presets");

    }
    else if (action->text() == "Сохранить" && action->property("root") == "Эксперимент"){
        if (experimentDirectory.isEmpty()){
            QString dir = QFileDialog::getExistingDirectory(this, tr("Open Directory"),
                                                        "/home",
                                                        QFileDialog::ShowDirsOnly
                                                                | QFileDialog::DontResolveSymlinks);
            this->experimentDirectory = dir;
        }
        //Сохранение файла подключений
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
                QString TCPCount =          (QString::number(info.serialInfo.tcpCount));
                QString TCPPort =           (info.serialInfo.tcpPort);

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
                QString port =              (info.tcpInfo.port);
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
        QFile connFile = QFile( experimentDirectory + '/' + "Configurations" + '/' + "connections.xml" );
        if( !connFile.open( QIODevice::WriteOnly | QIODevice::Text ) )
        {
            qDebug( "Failed to open file for writing." );
        }
        QTextStream connStream( &connFile );
        connStream.setCodec("UTF-8");
        connStream << connectionsDoc.toString();
        connFile.close();
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
        QFile eventFile = QFile( experimentDirectory + '/' + "Configurations" + '/' + "events.xml" );
        if( !eventFile.open( QIODevice::WriteOnly | QIODevice::Text ) )
        {
            qDebug( "Failed to open file for writing." );
        }
        QTextStream eventStream( &eventFile );
        eventStream.setCodec("UTF-8");
        eventStream << eventDoc.toString();
        eventFile.close();
    }
    else if (action->text() == "Конфигурация эксперимента"){
        experimentConfigurationDialog experimentDialog = experimentConfigurationDialog(experimentDirectory,this);
        experimentDialog.exec();
    }
    else if (action->text() == "Конфигурация устройств"){
        deviceConfigurationsDialog experimentDialog = deviceConfigurationsDialog(devicesMap, connectionsMap, messagesMap, this);
        experimentDialog.exec();
    }
}

void MainWindow::deleteConnection()
{
    if (ui->tableWidgetConnections->selectedItems().isEmpty()){
        QMessageBox::warning(this, "Ошибка", "Устройство не выбрано!");
        return;
    }
    int row = ui->tableWidgetConnections->currentRow();
    QString deviceName = ui->tableWidgetConnections->item(ui->tableWidgetConnections->currentRow(), 0)->text();
    if (connectionsMap.contains(deviceName)){
        QMessageBox::warning(this, "Ошибка", "Отключите устройство перед удалением!");
        return;
    }
    tableFieldsMap.remove(deviceName);
    foreach (auto key, tableFieldsMap.keys()) {
        TableConnectionsFields *fields = &tableFieldsMap[key];
        if (fields->row > row) fields->row -= 1;
    }
    if (devicesMap[deviceName].connType == "Serial" && devicesMap[deviceName].serialInfo.tcpCount > 0) bridgeMap.remove(deviceName);
    devicesMap.remove(deviceName);
    bufferMap.remove(deviceName);
    newDataMap.remove(deviceName);
    ui->tableWidgetConnections->removeRow(ui->tableWidgetConnections->currentRow());
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
    ConnectionSettings cs(deviceInfo, this);
    if (cs.exec() == QDialog::Accepted){
        DeviceInfo info = cs.getSettings();
        QString deviceName = info.ID;
        devicesMap[deviceName] = info;
    }
    fillConnectionsTable();
}


void MainWindow::openConnectionSettings()
{
    ConnectionSettings cs(this);
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
    QStringList fileNamesNew;
    for (auto note: notesList){
        QString tag = note.at(INDEX_NOTE_TAG);
        QString title = note.at(INDEX_NOTE_TITLE);
        QString body = note.at(INDEX_NOTE_BODY);
        QString isLapNote = note.at(INDEX_NOTE_ISLAP);
        QString fileName;
        if (isLapNote.toInt()){
            fileName = tag +"_Lap_"+lapNumber+"_"+title+".txt";
        }
        else{
            fileName = tag+"_General_"+title+".txt";
        }
        fileNamesNew.append(fileName);
        QFile file(experimentDirectory + '/' + "Notes" + '/' + fileName);
        if( !file.open( QIODevice::WriteOnly | QIODevice::Text ) )
        {
            qDebug( "Failed to open file for writing." );
        }
        QTextStream stream( &file );
        stream.setCodec("UTF-8");
        stream << body;
        file.close();
    }
    QDir directory(experimentDirectory + '/' + "Notes" + '/');
    QStringList fileNamesOld = directory.entryList(QDir::AllEntries | QDir::NoDotAndDotDot | QDir::Hidden);
    for(QString fileNameOld: fileNamesOld){
        if (!fileNamesNew.contains(fileNameOld)){
            QFile::remove(experimentDirectory + '/' + + "Notes" + '/' + fileNameOld);
        }
    }
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
    SSAD.exec();
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
        onnOffItem->setBackground(QBrush(QColor(0,255,0)));
        connectionsMap.insert(deviceName, connection);
        DeviceInfo info = devicesMap[deviceName];
        if (info.connType == "Serial" && info.serialInfo.tcpCount > 0){
            SerialToTcpBridge *bridge = new SerialToTcpBridge(info.serialInfo.tcpCount, info.serialInfo.tcpPort.toInt(), this);
            bridgeMap.insert(deviceName,bridge);
            connect(bridge, SIGNAL(newConnectionCount()), this, SLOT(onNewBridgeConnection()));
            bridge->start();
        }
        QByteArray* buffer = new QByteArray();
        bufferMap.insert(deviceName, buffer);
    }
    else if (protocolName == "TCP"){
        QTcpSocket* connection = new QTcpSocket(this);
        QString port = devicesMap[deviceName].tcpInfo.port;
        QString address = devicesMap[deviceName].tcpInfo.adress;

        // Подключаем сигналы
        connect(connection, &QTcpSocket::connected, this, [connection, onnOffItem, this, row, deviceName]() {
            onnOffItem->setBackground(QBrush(QColor(0,255,0)));
            this->connectionsMap.insert(deviceName, connection);
            QByteArray* buffer = new QByteArray();
            this->bufferMap.insert(deviceName, buffer);
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
    if (devicesMap[deviceName].connType == "Serial" && devicesMap[deviceName].serialInfo.tcpCount > 0){
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
    QString localTime = QTime::currentTime().toString("hh:mm:ss.zz");
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

void MainWindow::startExperiment()
{
    isLap = !isLap;
    if (isLap){
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
                    qDebug() << msg.toHex(' ');
                    parser.sendMessage(msg);
                }
                else if (event->protocol == "Unicore"){
                    UnicoreParser parser(connection);
                    qDebug() << event->message + "B 1";
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

void MainWindow::readPorts()
{
    foreach (QString connDevice, connectionsMap.keys()) {
        QObject* connection = connectionsMap[connDevice];
        QByteArray buff;
        if (connection == nullptr) continue;
        if (qobject_cast<QIODevice*>(connection)){
            QIODevice* ioCon = qobject_cast<QIODevice*>(connection);
            if (!ioCon->isOpen()) continue;

            if(ioCon->waitForReadyRead(1)){
                buff.append(ioCon->readAll());
                tableFieldsMap[connDevice].dataTimer = 10;
            }
        }
        if (buff.isEmpty()) return;
        NewData data;
        data.deviceInfo = devicesMap[connDevice];
        data.buff = buff;
        newDataMap[connDevice] = data;
        if (data.deviceInfo.connType == "Serial" && data.deviceInfo.serialInfo.tcpCount > 0){
            SerialToTcpBridge* bridge = bridgeMap[connDevice];
            bridge->write(buff);
        }
        bufferMap[connDevice]->append(buff);
        emit newData(connDevice);
    }
}

QMap<QString,NewData> MainWindow::getNewData(){
    return newDataMap;
}

void MainWindow::parseMessage()
{
    if (!isLap) return;
    QTime currTime = QTime::currentTime();
    int currSeconds = currTime.second() + currTime.minute()*60 + currTime.hour()*3600;
    int startSeconds = lapTime.second() + lapTime.minute()*60 + lapTime.hour()*3600;
    int diffSeconds = currSeconds - startSeconds;
    ui->labelElapsedTime->setText(QString::number(diffSeconds) + currTime.toString(".zzz").left(3));

    foreach (QString connDevice, connectionsMap.keys()) {
        QObject* connection = connectionsMap[connDevice];
        QByteArray *buff = bufferMap[connDevice];
        if (connection == nullptr) continue;
        if (!isLap) continue;
        if (buff->isEmpty()) continue;
        QByteArray messData;
        QString messId;
        QString GNSSTime;
        QDataStream::ByteOrder order;
        DeviceInfo deviceInfo = devicesMap[connDevice];
        QString protocol = deviceInfo.protocol;
        if (protocol == "Ublox"){
            UbloxParser parser(connection);
            UbloxMessage mess = parser.parseMessage(buff);
            if (mess.data.isEmpty()) continue;
            messData = mess.data;
            messId = QString::number(mess.messClass, 16).rightJustified(2,'0') + QString::number(mess.messId, 16).rightJustified(2,'0');
            order = QDataStream::LittleEndian;
        }
        else if (protocol == "Unicore"){
            UnicoreParser parser(connection);
            UnicoreMessage mess = parser.parseBinaryMessage(buff);
            if (mess.data.isEmpty()) continue;
            messData = mess.data;
            messId = QString::number(mess.binaryHeader.messageId);
            GNSSTime = QString::number(mess.binaryHeader.ms);
            order = QDataStream::BigEndian;
        }
        else continue;
        foreach (QString eventName, eventMap.keys()) {
            EventData *event = &eventMap[eventName];
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
            qDebug() << '\n';
            qDebug() << ("event: " + event->name) << ("field: " + event->fieldName) << ("type: " + event->fieldType);
            if (event->fieldType == "int" || event->fieldType == "uint" || event->fieldType == "float" || event->fieldType == "double"){
                int flags = (int)event->intTriggers.isEqual +
                            ((int)event->intTriggers.isGreater << 1) +
                            ((int)event->intTriggers.isLesser << 2);
                double thresh = event->intTriggers.threshhold / scale;
                if (event->fieldType == "int" && size == 1) check = compareValue<qint8>(stream, thresh, flags, event->fieldType);
                else if (event->fieldType == "uint" && size == 1) check = compareValue<quint8>(stream, thresh, flags, event->fieldType);
                else if (event->fieldType == "int" && size == 2) check = compareValue<qint16>(stream, thresh, flags, event->fieldType);
                else if (event->fieldType == "uint" && size == 2) check = compareValue<quint16>(stream, thresh, flags, event->fieldType);
                else if (event->fieldType == "int" && size == 4) check = compareValue<qint32>(stream, thresh, flags, event->fieldType);
                else if (event->fieldType == "uint" && size == 4) check = compareValue<quint32>(stream, thresh, flags, event->fieldType);
                else if (event->fieldType == "int" && size == 8) check = compareValue<qint64>(stream, thresh, flags, event->fieldType);
                else if (event->fieldType == "uint" && size == 8) check = compareValue<quint64>(stream, thresh, flags, event->fieldType);
                else if (event->fieldType == "float") check = compareValue<float>(stream, thresh, flags, event->fieldType);
                else if (event->fieldType == "double") check = compareValue<double>(stream, thresh, flags, event->fieldType);
                else if (event->fieldType == "int") check = compareValue<qint32>(stream, thresh, flags, event->fieldType);
                else if (event->fieldType == "uint") check = compareValue<quint32>(stream, thresh, flags, event->fieldType);
                else {
                    qWarning() << "Unsupported field type:" << event->fieldType;
                }
            }
            else if (event->fieldType == "char"){
                qDebug() << ("data: " + QString::fromLatin1(data)) << ("event value: " + event->charTriggers.charValue);
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
                    QString localTime = QTime::currentTime().toString("hh:mm:ss.zz");
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


