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

/// Мысли по поводу принципа работы приложения
/// Чтение с портов:
/// Чтение с портов происходит только в одном месте*, а в остальные окна отправляется уже обработанная информация
/// Читаем порт, запускаем парсеры, парсим сообщения получаем наследников структуры Message, закидываем их в массив
/// При запуске какого либо из окон передаем ему также ссылку на этот массив, с ним же и работаем
///
/// *В конфигурацию устройств пользователь не должен заходить посередине захода, следовательно там можно читать с порта
/// *Во вкладке териминала нужно предупеждать о завершении захода

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

/// Задачи:
/// 1. Добавить для U-blox функцию, расшифровывающую сообщение в строку
/// 2. Изменить интерфейс окна настройки событий
/// 3. Разработать логику кастомных событий
///
/// План:
/// 1. Реализовать для Unicore
/// 2. Добавить для U-blox функцию
/// 3. Реализовать для U-blox
///
/// Соотнесение индекса поля со строкой:
/// у Unicore всё просто - нужно данные разделить по запятой, а затем соотносить по списку
/// у U-blox очень сложно - как вариант можно добавить функцию, которая преобразует данные в строку, подобную Unicore сообщению,
/// а дальше также.
/// 3. Работа по протоколу. Сделано, мыслей нет
/// 6. “Конфигурация устройств”, “Основные настройки”.
/// Откуда я должен достать ограничения антенны? У антены есть только путь к файлу, какая инфа хранится в нем?
/// Как это реализовывать? Может проще отправить пользователя в работу по протоколу?
/// Чем имеющаяся реализация хуже? Она же имеет тот же самый функционал, за исключением непонятного чекбокса антенны
/// 8. Добавление устройств по файлу. В целом легко, можно добавить кнопку "добавить из файла", а дальше тот же самый
/// функционал, что у действия "загрузить"


/// При изменении/удалении чего либо имеющего свой файл нужно "на ходу" изменить этот файл !!!! надо уточнить !!!!
/// TODO: Загрузка эксперимента
///
/// TODO: Сохранение эксперимента
///
/// TODO: Логика событий
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

    QTimer *timer = new QTimer(this);
    connect(timer, SIGNAL(timeout()), this, SLOT(parseMessage()));
    timer->start(1);
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::addItemToConnectionsTable(QString protocol, QList<QString> parameters)
{
    int rowCount = ui->tableWidgetConnections->rowCount();
    ui->tableWidgetConnections->setRowCount(rowCount+1);
    QString deviceName = parameters.at(INDEX_SERIAL_ID);
    if (protocol == "Serial"){
        QString transferProtocol =  parameters.at(INDEX_GENERAL_PROTOCOL);
        QString deviceType =        parameters.at(INDEX_GENERAL_DEVICE_TYPE);
        QString port =              parameters.at(INDEX_SERIAL_PORT);
        QString baudrate =          parameters.at(INDEX_SERIAL_BAUDRATE);
        QString dataBits =          parameters.at(INDEX_SERIAL_DATA_BITS);
        QString TCPPort =           parameters.at(INDEX_SERIAL_TCP_PORT);
        QString parity =            parameters.at(INDEX_SERIAL_PARITY);
        QString stopBits =          parameters.at(INDEX_SERIAL_STOP_BITS);
        QString TCPCount =          parameters.at(INDEX_SERIAL_TCP_COUNT);

        QTableWidgetItem *idItem = new QTableWidgetItem(deviceName);
        ui->tableWidgetConnections->setItem(rowCount,INDEX_CONN_TABLE_ID,idItem);
        QTableWidgetItem *typeItem = new QTableWidgetItem(protocol);
        ui->tableWidgetConnections->setItem(rowCount,INDEX_CONN_TABLE_TYPE,typeItem);
        QTableWidgetItem *portItem = new QTableWidgetItem(TCPPort);
        ui->tableWidgetConnections->setItem(rowCount,INDEX_CONN_TABLE_TCP_PORT,portItem);
    }
    else if (protocol == "CAN"){
        QString transferProtocol =  parameters.at(INDEX_GENERAL_PROTOCOL);
        QString deviceType =        parameters.at(INDEX_GENERAL_DEVICE_TYPE);
        QString baudrate =           parameters.at(INDEX_CAN_BAUDRATE);
        QString CANType =           parameters.at(INDEX_CAN_TYPE);

        QTableWidgetItem *idItem = new QTableWidgetItem(deviceName);
        ui->tableWidgetConnections->setItem(rowCount,INDEX_CONN_TABLE_ID,idItem);
        QTableWidgetItem *typeItem = new QTableWidgetItem(protocol);
        ui->tableWidgetConnections->setItem(rowCount,INDEX_CONN_TABLE_TYPE,typeItem);
    }
    else if (protocol == "TCP"){
        QString transferProtocol =  parameters.at(INDEX_GENERAL_PROTOCOL);
        QString deviceType =        parameters.at(INDEX_GENERAL_DEVICE_TYPE);
        QString clientServer =      parameters.at(INDEX_TCP_CLIENT_SERVER);
        QString port =              parameters.at(INDEX_TCP_PORT);
        QString adress =            parameters.at(INDEX_TCP_ADRESS);

        QTableWidgetItem *idItem = new QTableWidgetItem(deviceName);
        ui->tableWidgetConnections->setItem(rowCount,INDEX_CONN_TABLE_ID,idItem);
        QTableWidgetItem *typeItem = new QTableWidgetItem(protocol);
        ui->tableWidgetConnections->setItem(rowCount,INDEX_CONN_TABLE_TYPE,typeItem);
        QTableWidgetItem *portItem = new QTableWidgetItem(port);
        ui->tableWidgetConnections->setItem(rowCount,INDEX_CONN_TABLE_TCP_PORT,portItem);
    }

    QTableWidgetItem *onnOffItem = new QTableWidgetItem;
    onnOffItem->setBackground(QBrush(QColor(255,0,0)));
    ui->tableWidgetConnections->setItem(rowCount, INDEX_CONN_TABLE_ON_OFF, onnOffItem);
    QTableWidgetItem *dataItem = new QTableWidgetItem;
    dataItem->setBackground(QBrush(QColor(0,100,0)));
    ui->tableWidgetConnections->setItem(rowCount, INDEX_CONN_TABLE_DATA, dataItem);

    setupTableSize(ui->tableWidgetConnections);
}

void MainWindow::fillConnectionsTable()
{
    ui->tableWidgetConnections->clearContents();
    ui->tableWidgetConnections->setRowCount(0);
    for (const auto &key: devicesMap.keys()) {
        QString deviceName = key;
        QPair<QString,QList<QString>> settings = devicesMap.value(key);
        QString protocolName = settings.first;
        QList<QString> parameters = settings.second;
        addItemToConnectionsTable(protocolName,settings.second);
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
        QDomElement e = deviceXml.toElement();
        QString deviceName = e.tagName();
        QDomElement parametersXml = deviceXml.firstChild().toElement();
        QString protocolName = parametersXml.tagName();
        QList<QString> parameters;
        if (protocolName == "Serial"){
            QString transferProtocol =  parametersXml.attribute("transfer_protocol");
            QString deviceType =        parametersXml.attribute("device_type");
            QString port =              parametersXml.attribute("Serial_port");
            QString baudrate =          parametersXml.attribute("baudrate");
            QString dataBits =          parametersXml.attribute("data_bits");
            QString TCPPort =           parametersXml.attribute("TCP_port_number");
            QString parity =            parametersXml.attribute("parity");
            QString stopBits =          parametersXml.attribute("stop_bits");
            QString TCPCount =          parametersXml.attribute("TCP_connections_number");

            parameters << deviceName << deviceType << transferProtocol << port << baudrate << dataBits << TCPPort << parity << stopBits << TCPCount;

        }
        else if (protocolName == "CAN"){
            QString transferProtocol =  parametersXml.attribute("transfer_protocol");
            QString deviceType =        parametersXml.attribute("device_type");
            QString baudrate =           parametersXml.attribute("baudrate");
            QString CANType =           parametersXml.attribute("CAN_type");

            parameters << deviceName << deviceType << transferProtocol  << baudrate << CANType;
        }
        else if (protocolName == "TCP"){
            QString transferProtocol =  parametersXml.attribute("transfer_protocol");
            QString deviceType =        parametersXml.attribute("device_type");
            QString clientServer =      parametersXml.attribute("source");
            QString port =              parametersXml.attribute("port_number");
            QString adress =            parametersXml.attribute("adress");

            parameters << deviceName << deviceType << transferProtocol  << clientServer << port << adress;
        }
        QPair<QString,QList<QString>> protocolPair;
        protocolPair.first = protocolName;
        protocolPair.second = parameters;
        if(!devicesMap.contains(deviceName)){
            devicesMap.insert(deviceName,protocolPair);
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
            QDomElement e = deviceXml.toElement();
            QString deviceName = e.tagName();
            QDomElement parametersXml = deviceXml.firstChild().toElement();
            QString protocolName = parametersXml.tagName();
            QList<QString> parameters;
            if (protocolName == "Serial"){
                QString transferProtocol =  parametersXml.attribute("transfer_protocol");
                QString deviceType =        parametersXml.attribute("device_type");
                QString port =              parametersXml.attribute("Serial_port");
                QString baudrate =          parametersXml.attribute("baudrate");
                QString dataBits =          parametersXml.attribute("data_bits");
                QString TCPPort =           parametersXml.attribute("TCP_port_number");
                QString parity =            parametersXml.attribute("parity");
                QString stopBits =          parametersXml.attribute("stop_bits");
                QString TCPCount =          parametersXml.attribute("TCP_connections_number");

                parameters << deviceName << deviceType << transferProtocol << port << baudrate << dataBits << TCPPort << parity << stopBits << TCPCount;

            }
            else if (protocolName == "CAN"){
                QString transferProtocol =  parametersXml.attribute("transfer_protocol");
                QString deviceType =        parametersXml.attribute("device_type");
                QString baudrate =           parametersXml.attribute("baudrate");
                QString CANType =           parametersXml.attribute("CAN_type");

                parameters << deviceName << deviceType << transferProtocol  << baudrate << CANType;
            }
            else if (protocolName == "TCP"){
                QString transferProtocol =  parametersXml.attribute("transfer_protocol");
                QString deviceType =        parametersXml.attribute("device_type");
                QString clientServer =      parametersXml.attribute("source");
                QString port =              parametersXml.attribute("port_number");
                QString adress =            parametersXml.attribute("adress");

                parameters << deviceName << deviceType << transferProtocol  << clientServer << port << adress;
            }
            QPair<QString,QList<QString>> protocolPair;
            protocolPair.first = protocolName;
            protocolPair.second = parameters;
            if(!devicesMap.contains(deviceName)){
                devicesMap.insert(deviceName,protocolPair);
            }
            deviceXml = deviceXml.nextSibling();
        }
        fillConnectionsTable();
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
        setupTableSize(ui->tableWidgetConnections);
        QDir experimentDir(experimentDirectory);
        experimentDir.mkpath(experimentDirectory + '/' + "Configurations" + '/' + "Experiment_configurations");
        experimentDir.mkpath(experimentDirectory + '/' + "Configurations" + '/' + "Device_configurations");
        experimentDir.mkpath(experimentDirectory + '/' + "Configurations" + '/' + "Lap_presets");
        QString fieldsDir = dir + '/' + "Messages_fields.conf";
        QFile fieldsFile(fieldsDir);
        if (fieldsFile.open(QIODevice::ReadOnly)){
            while (true){
                QByteArray line = fieldsFile.readLine();
                if (fieldsFile.atEnd()) break;
                QString name = QString::fromUtf8(line.split(':').at(1));
                QList<QByteArray> fields = line.split(':').at(2).split(',');
                QString protocol = QString::fromUtf8(line.split(':').at(0));
                QStringList fieldsStr;
                QPair<QString,QStringList> pair;
                foreach (QByteArray field, fields) {
                    fieldsStr.append(QString::fromUtf8(field));
                }
                pair.first = protocol;
                pair.second = fieldsStr;
                fieldsMap[name] = pair;
            }
        }
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
            QPair<QString,QList<QString>> settings = devicesMap.value(key);
            QString protocolName = settings.first;
            if (protocolName == "Serial"){
                QString transferProtocol =  settings.second.at(INDEX_GENERAL_PROTOCOL);
                QString deviceType =        settings.second.at(INDEX_GENERAL_DEVICE_TYPE);
                QString port =              settings.second.at(INDEX_SERIAL_PORT);
                QString baudrate =          settings.second.at(INDEX_SERIAL_BAUDRATE);
                QString dataBits =          settings.second.at(INDEX_SERIAL_DATA_BITS);
                QString TCPPort =           settings.second.at(INDEX_SERIAL_TCP_PORT);
                QString parity =            settings.second.at(INDEX_SERIAL_PARITY);
                QString stopBits =          settings.second.at(INDEX_SERIAL_STOP_BITS);
                QString TCPCount =          settings.second.at(INDEX_SERIAL_TCP_COUNT);

                QDomElement deviceXml = connectionsDoc.createElement(deviceName);
                connectionsRootElement.appendChild(deviceXml);

                QDomElement protocolXml = connectionsDoc.createElement(protocolName);
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
            else if (protocolName == "CAN"){
                QString transferProtocol =  settings.second.at(INDEX_GENERAL_PROTOCOL);
                QString deviceType =        settings.second.at(INDEX_GENERAL_DEVICE_TYPE);
                QString baudrate =           settings.second.at(INDEX_CAN_BAUDRATE);
                QString CANType =           settings.second.at(INDEX_CAN_TYPE);

                QDomElement deviceXml = connectionsDoc.createElement(deviceName);
                connectionsRootElement.appendChild(deviceXml);

                QDomElement protocolXml = connectionsDoc.createElement(protocolName);
                protocolXml.setAttribute("device_type", deviceType);
                protocolXml.setAttribute("transfer_protocol", transferProtocol);
                protocolXml.setAttribute("baudrate", baudrate);
                protocolXml.setAttribute("CAN_type", CANType);
                deviceXml.appendChild(protocolXml);
            }
            else if (protocolName == "TCP"){
                QString transferProtocol =  settings.second.at(INDEX_GENERAL_PROTOCOL);
                QString deviceType =        settings.second.at(INDEX_GENERAL_DEVICE_TYPE);
                QString clientServer =      settings.second.at(INDEX_TCP_CLIENT_SERVER);
                QString port =              settings.second.at(INDEX_TCP_PORT);
                QString adress =            settings.second.at(INDEX_TCP_ADRESS);

                QDomElement deviceXml = connectionsDoc.createElement(deviceName);
                connectionsRootElement.appendChild(deviceXml);

                QDomElement protocolXml = connectionsDoc.createElement(protocolName);
                protocolXml.setAttribute("device_type", deviceType);
                protocolXml.setAttribute("transfer_protocol", transferProtocol);
                protocolXml.setAttribute("source", clientServer);
                protocolXml.setAttribute("port_number", port);
                protocolXml.setAttribute("adress", adress);
                deviceXml.appendChild(protocolXml);
            }
        }
        QFile file( experimentDirectory + '/' + "Configurations" + '/' + "connections.xml" );
        if( !file.open( QIODevice::WriteOnly | QIODevice::Text ) )
        {
            qDebug( "Failed to open file for writing." );
        }
        QTextStream stream( &file );
        stream.setCodec("UTF-8");
        stream << connectionsDoc.toString();
        file.close();
    }
    else if (action->text() == "Конфигурация эксперимента"){
        experimentConfigurationDialog experimentDialog = experimentConfigurationDialog(this);
        experimentDialog.exec();
    }
    else if (action->text() == "Конфигурация устройств"){
        if(isLap){
            QMessageBox::warning(this, "Ошибка", "Нельзя конфигурировать устройства во время захода!\nЗавершите заход!");
            return;
        }
        deviceConfigurationsDialog experimentDialog = deviceConfigurationsDialog(devicesMap,connectionsMap,this);
        experimentDialog.exec();
    }


}

void MainWindow::deleteConnection()
{
    if (ui->tableWidgetConnections->selectedItems().isEmpty()){
        QMessageBox::warning(this, "Ошибка", "Устройство не выбрано!");
        return;
    }
    QString deviceName = ui->tableWidgetConnections->item(ui->tableWidgetConnections->currentRow(), 0)->text();
    if (connectionsMap.contains(deviceName)){
        QMessageBox::warning(this, "Ошибка", "Отключите устройство перед удалением!");
        return;
    }
    devicesMap.remove(deviceName);
    ui->tableWidgetConnections->removeRow(ui->tableWidgetConnections->currentRow());
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

    QPair<QString,QList<QString>> deviceInfo = devicesMap[deviceName];
    ConnectionSettings cs(deviceInfo, this);
    if (cs.exec() == QDialog::Accepted){
        QPair<QString,QList<QString>> settings = cs.getSettings();
        QString deviceName = settings.second.at(INDEX_GENERAL_ID);
        QString protocolName = settings.first;
        QList<QString> parameters;
        QString transferProtocol = settings.second.at(INDEX_GENERAL_PROTOCOL);
        QString deviceType = settings.second.at(INDEX_GENERAL_DEVICE_TYPE);
        parameters << deviceName << deviceType << transferProtocol;

        if(protocolName == "Serial"){
            QString port =      settings.second.at(INDEX_SERIAL_PORT);
            QString baudrate =  settings.second.at(INDEX_SERIAL_BAUDRATE);
            QString dataBits =  settings.second.at(INDEX_SERIAL_DATA_BITS);
            QString TCPPort =   settings.second.at(INDEX_SERIAL_TCP_PORT);
            QString parity =    settings.second.at(INDEX_SERIAL_PARITY);
            QString stopBits =  settings.second.at(INDEX_SERIAL_STOP_BITS);
            QString TCPCount =  settings.second.at(INDEX_SERIAL_TCP_COUNT);

            parameters << port << baudrate << dataBits << TCPPort << parity << stopBits << TCPCount;

            QTableWidgetItem *portItem = new QTableWidgetItem(TCPPort);
            ui->tableWidgetConnections->setItem(ui->tableWidgetConnections->currentRow(), INDEX_CONN_TABLE_TCP_PORT, portItem);
        }
        else if(protocolName == "CAN"){
            QString baudrate = settings.second.at(INDEX_CAN_BAUDRATE);
            QString CANType = settings.second.at(INDEX_CAN_TYPE);

            parameters << baudrate << CANType;

        }
        else if(protocolName == "TCP"){
            QString clientServer =  settings.second.at(INDEX_TCP_CLIENT_SERVER);
            QString port =          settings.second.at(INDEX_TCP_PORT);
            QString adress =        settings.second.at(INDEX_TCP_ADRESS);

            parameters << clientServer << port << adress;
            QTableWidgetItem *portItem = new QTableWidgetItem(port);
            ui->tableWidgetConnections->setItem(ui->tableWidgetConnections->currentRow(), INDEX_CONN_TABLE_TCP_PORT, portItem);
        }
        QPair<QString,QList<QString>> protocolPair;
        protocolPair.first = protocolName;
        protocolPair.second = parameters;
        devicesMap[deviceName] = protocolPair;
        // вывод списка
        // qDebug() << "------------------------------------";
        // dumpSimpleMap<QString,QPair<QString,QList<QString>>>(devicesMap);
        // qDebug() << "------------------------------------";

        // addItemToConnectionsTable(protocolName, settings.second);
    }
}
/*
Есть несколько устройств с разными способами подключения к компу. При добавлении их в программе, инфа о них должна храниться в оперативке и конфиге.
То есть нужно при нажатии на ок в окне добавления получить всю информацию о новом подключении, добавить её в соответствующий массив и в файл.
Если устройства с таким названием раньше не было, то нужно создать новый экземпляр в массиве и файле, а если было, то изменить существующий.
предполагаю что лучше всего будет использовать словари, т.к. названия подключений и устройств предполагаются уникальными для каждого устройства
[имя устройсва; [имя типа подключения; {параметры}]
хотим получить параметры конкретного подключения у ус-ва: обращаемся по ключу устройства -> получаем пару из названия протокола и параметров
инфу о которых можно получить через enum соответствущего подключения
QMap<QString,QPair<QString,QList<QString>>>
 ^      ^            ^          ^
список ус-во      протокол    параметры
Проблема: как работать с двумерным QMap?
Добавление в корень : insert(QString,Qmap<QString,QString>)
Добавление в ветку : m["key"].insert("key","value")
Изменение в ветке : m["key"]["key"] = "value"
Удаление элемента : m.remove["key"]      m["key"].remove("key")
Конфиг:
<Connections>
    <DeviceID>
        <Serial
            device_type="Приемник"
            transfer_protocol="Ublox"
            baudrate="4800"
            data_bits="7"
            TCP_port_number="5000"
            parity="Четное"
            stop_bits="1"
            TCP_connections_number="5"
            />
    </DeviceID>
</Connections>
*/


void MainWindow::openConnectionSettings()
{
    ConnectionSettings cs(this);
    if (cs.exec() == QDialog::Accepted){
        QPair<QString,QList<QString>> settings = cs.getSettings();
        QString deviceName = settings.second.at(INDEX_GENERAL_ID);
        if(devicesMap.contains(settings.second.at(INDEX_GENERAL_ID))){
            QMessageBox::warning(this, "Ошибка", "Устройство с таким ID уже добавлено!\nУдалите или измените имеющееся если хотите добавить это");
            return;
        }
        QString protocolName = settings.first;
        QList<QString> parameters;
        QString transferProtocol = settings.second.at(INDEX_GENERAL_PROTOCOL);
        QString deviceType = settings.second.at(INDEX_GENERAL_DEVICE_TYPE);
        parameters << deviceName << deviceType << transferProtocol;

        if(protocolName == "Serial"){
            QString port =      settings.second.at(INDEX_SERIAL_PORT);
            QString baudrate =  settings.second.at(INDEX_SERIAL_BAUDRATE);
            QString dataBits =  settings.second.at(INDEX_SERIAL_DATA_BITS);
            QString TCPPort =   settings.second.at(INDEX_SERIAL_TCP_PORT);
            QString parity =    settings.second.at(INDEX_SERIAL_PARITY);
            QString stopBits =  settings.second.at(INDEX_SERIAL_STOP_BITS);
            QString TCPCount =  settings.second.at(INDEX_SERIAL_TCP_COUNT);

            parameters << port << baudrate << dataBits << TCPPort << parity << stopBits << TCPCount;

        }
        else if(protocolName == "CAN"){
            QString baudrate = settings.second.at(INDEX_CAN_BAUDRATE);
            QString CANType = settings.second.at(INDEX_CAN_TYPE);

            parameters << baudrate << CANType;

        }
        else if(protocolName == "TCP"){
            QString clientServer =  settings.second.at(INDEX_TCP_CLIENT_SERVER);
            QString port =          settings.second.at(INDEX_TCP_PORT);
            QString adress =        settings.second.at(INDEX_TCP_ADRESS);

            parameters << clientServer << port << adress;

        }
        QPair<QString,QList<QString>> protocolPair;
        protocolPair.first = protocolName;
        protocolPair.second = parameters;
        if(!devicesMap.contains(deviceName)){
            devicesMap.insert(deviceName,protocolPair);
        }
        // вывод списка
        // qDebug() << "------------------------------------";
        // dumpSimpleMap<QString,QPair<QString,QList<QString>>>(devicesMap);
        // qDebug() << "------------------------------------";

        addItemToConnectionsTable(protocolName, settings.second);
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
    eventSettingsDialog eSD = eventSettingsDialog(&eventMap, devicesMap, fieldsMap, this);
    eSD.exec();
    foreach (QString key, eventMap.keys()) {
        eventData data = eventMap[key];
        qDebug() << data.device << data.message << data.fieldName << data.field  << data.protocol;
    }
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
    QTableWidgetItem *onnOffItem = new QTableWidgetItem;
    onnOffItem->setBackground(QBrush(QColor(255,255,0)));
    ui->tableWidgetConnections->setItem(row, INDEX_CONN_TABLE_ON_OFF, onnOffItem);
    if (protocolName == "Serial"){
        QString port;
        int baudrate;
        QSerialPort* connection = new QSerialPort(this);
        QSerialPort::Parity parity = QSerialPort::NoParity;
        QString parityStr = devicesMap[deviceName].second.at(INDEX_SERIAL_PARITY);
        if (parityStr == "Нет"){
            parity = QSerialPort::NoParity;
        }
        if (parityStr == "Четное"){
            parity = QSerialPort::EvenParity;
        }
        if (parityStr == "Нечетное"){
            parity = QSerialPort::OddParity;
        }
        int dataBits = devicesMap[deviceName].second.at(INDEX_SERIAL_DATA_BITS).toInt();
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
        int stopBits = devicesMap[deviceName].second.at(INDEX_SERIAL_STOP_BITS).toInt();
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
        port = devicesMap[deviceName].second.at(INDEX_SERIAL_PORT);
        baudrate = devicesMap[deviceName].second.at(INDEX_SERIAL_BAUDRATE).toInt();
        connection->setPortName(port);
        connection->setBaudRate(baudrate);
        // пробуем подключится
        if (!connection->open(QIODevice::ReadWrite)) {
            onnOffItem->setBackground(QBrush(QColor(255,0,0)));
            ui->tableWidgetConnections->setItem(row, INDEX_CONN_TABLE_ON_OFF, onnOffItem);
            QMessageBox::warning(this, "Ошибка", "Не удалось подключится к порту");
            return;
        }
        else{
            onnOffItem->setBackground(QBrush(QColor(0,255,0)));
            ui->tableWidgetConnections->setItem(row, INDEX_CONN_TABLE_ON_OFF, onnOffItem);
            connectionsMap.insert(deviceName, connection);
            QByteArray buffer;
            bufferMap.insert(deviceName, buffer);
            QMap<QString,int> flags;
            flags["RTK Sol found"] = INDEX_FLAGS_UNKNOWN;
            flags["RTK Sol lost"] = INDEX_FLAGS_UNKNOWN;
            flags["RTK Rel Sol found"] = INDEX_FLAGS_UNKNOWN;
            flags["RTK Rel Sol lost"] = INDEX_FLAGS_UNKNOWN;
            flagsMap.insert(deviceName,flags);
        }
    }
    else if (protocolName == "TCP"){
        QTcpSocket* connection = new QTcpSocket(this);
        QString port = devicesMap[deviceName].second.at(INDEX_TCP_PORT);
        QString address = devicesMap[deviceName].second.at(INDEX_TCP_ADRESS);

        // Подключаем сигналы
        connect(connection, &QTcpSocket::connected, this, [connection, onnOffItem, this, row, deviceName]() {
            onnOffItem->setBackground(QBrush(QColor(0,255,0)));
            this->ui->tableWidgetConnections->setItem(row, INDEX_CONN_TABLE_ON_OFF, onnOffItem);
            this->connectionsMap.insert(deviceName, connection);
            QByteArray buffer;
            this->bufferMap.insert(deviceName, buffer);
            QMap<QString,int> flags;
            flags["RTK Sol found"] = INDEX_FLAGS_UNKNOWN;
            flags["RTK Sol lost"] = INDEX_FLAGS_UNKNOWN;
            flags["RTK Rel Sol found"] = INDEX_FLAGS_UNKNOWN;
            flags["RTK Rel Sol lost"] = INDEX_FLAGS_UNKNOWN;
            this->flagsMap.insert(deviceName,flags);
        });

        connect(connection, &QTcpSocket::disconnected, this, [connection, onnOffItem, this, row]() {
            onnOffItem->setBackground(QBrush(QColor(255,0,0)));
            ui->tableWidgetConnections->setItem(row, INDEX_CONN_TABLE_ON_OFF, onnOffItem);
            // QMessageBox::warning(this, "Ошибка", "Не удалось подключится к порту");
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
    QTableWidgetItem *onnOffItem = new QTableWidgetItem;
    if (!connectionsMap.contains(deviceName)){
        QMessageBox::warning(this, "Ошибка", "Это устройство не подключено!");
        onnOffItem->setBackground(QBrush(QColor(255,0,0)));
        ui->tableWidgetConnections->setItem(row, INDEX_CONN_TABLE_ON_OFF, onnOffItem);
        connectionsMap.remove(ui->tableWidgetConnections->item(row,INDEX_CONN_TABLE_ID)->text());
        return;
    }
    QObject* connection = connectionsMap[deviceName];
    if (qobject_cast<QIODevice*>(connection)){
        QIODevice* ioCon = qobject_cast<QIODevice*>(connection);
        ioCon->close();
    }
    onnOffItem->setBackground(QBrush(QColor(255,0,0)));
    ui->tableWidgetConnections->setItem(row, INDEX_CONN_TABLE_ON_OFF, onnOffItem);
    flagsMap.remove(deviceName);
    bufferMap.remove(deviceName);
    connectionsMap.remove(deviceName);
}

void MainWindow::sendUserEvent()
{
    if(!isLap) return;
    QString localTime = QTime::currentTime().toString("hh:mm:ss.zz");
    QString event = ui->lineEditAddEvent->text();
    addItemToLogTable(localTime, "", event);
}

/// TODO:: логика старта эксперимента
/// При старте в лог пишется событие "эксперимент начат"
/// При остановке в лог пишется событие "эксперимент завершен"
/// В зависимости от заданных флагов в окне настроек лога должны выводиться соответствующие события
/// Нужен таймер, и парсинг сообщений наподобие того, что в deviceConfigurations, только ограниченный парой сообщений
/// fix решение найдено:
/// Для ublox это NAV-SOL поле gps-fix меняется с нуля на не ноль
///
/// решение потеряно:
/// Для ublox это NAV-SOL поле gps-fix меняется с не нуля на ноль
/// Для таких сообщений должна быть примерно такая форма <ID устройства>: <тело сообщения>

void MainWindow::addItemToLogTable(QString localTime, QString GNSSTime, QString event)
{
    int row = ui->tableWidgetLog->rowCount()+1;
    ui->tableWidgetLog->setRowCount(row);
    QTableWidgetItem* localTimeItem = new QTableWidgetItem(localTime);
    QTableWidgetItem* GNSSTimeItem = new QTableWidgetItem(GNSSTime);
    QTableWidgetItem* eventItem = new QTableWidgetItem(event);
    ui->tableWidgetLog->setItem(row-1,0,localTimeItem);
    ui->tableWidgetLog->setItem(row-1,1,GNSSTimeItem);
    ui->tableWidgetLog->setItem(row-1,2,eventItem);
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
        if (eventSettingsSolFound || eventSettingsSolLost){
            foreach (QString key, connectionsMap.keys()) {
                QObject* connection = connectionsMap[key];
                QByteArray buff = bufferMap[key];
                QPair<QString,QList<QString>> deviceInfo = devicesMap[key];
                QString protocol = deviceInfo.second.at(INDEX_GENERAL_PROTOCOL);
                if (qobject_cast<QIODevice*>(connection)){
                    QIODevice* ioCon = qobject_cast<QIODevice*>(connection);
                    if (!ioCon->isOpen()) return;
                    if(ioCon->waitForReadyRead(1)){
                        buff.append(ioCon->readAll());
                    }
                }
                if (buff.isEmpty()) continue;
                if (protocol == "Ublox"){
                    UbloxParser parser(connection);
                    QByteArray msg;
                    QByteArray hdr;
                    QByteArray checkSum;
                    hdr.resize(2);
                    hdr[0] = 0xb5;
                    hdr[1] = 0x62;
                    QByteArray en;
                    en.resize(4);
                    en[0] = CFG::MSG::classID;
                    en[1] = CFG::MSG::messageID;
                    en[2] = 0x08;
                    en[3] = 0x00;
                    QByteArray payload;
                    payload.resize(8);
                    payload[2] = 0x00;
                    payload[3] = 0x00;
                    payload[4] = 0x00;
                    payload[5] = 0x01;
                    payload[6] = 0x00;
                    payload[7] = 0x00;

                    payload[0] = NAV::SOL::classID;
                    payload[1] = NAV::SOL::messageID;
                    msg = hdr + en + payload;
                    checkSum = UbloxParser::calcCheckSum(en + payload);
                    msg.append(checkSum);
                    parser.sendMessage(msg);
                    payload[0] = NAV::RELPOSNED::classID;
                    payload[1] = NAV::RELPOSNED::messageID;
                    msg = hdr + en + payload;
                    checkSum = UbloxParser::calcCheckSum(en + payload);
                    msg.append(checkSum);
                    parser.sendMessage(msg);
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
        dumpNestedMap(flagsMap);
        foreach (QString key, flagsMap.keys()) {
            foreach (QString flag, flagsMap[key].keys()) {
                flagsMap[key][flag] = INDEX_FLAGS_UNKNOWN;
            }
        }
    }
}

void MainWindow::parseMessage()
{
    if (!isLap) return;
    QTime currTime = QTime::currentTime();
    int currSeconds = currTime.second() + currTime.minute()*60 + currTime.hour()*3600;
    int startSeconds = lapTime.second() + lapTime.minute()*60 + lapTime.hour()*3600;
    int diffSeconds = currSeconds - startSeconds;
    ui->labelElapsedTime->setText(QString::number(diffSeconds) + currTime.toString(".zzz").left(3));
    foreach (QString key, connectionsMap.keys()) {
        QObject* connection = connectionsMap[key];
        QByteArray buff = bufferMap[key];
        QPair<QString,QList<QString>> deviceInfo = devicesMap[key];
        QString protocol = deviceInfo.second.at(INDEX_GENERAL_PROTOCOL);
        if (connection == nullptr) continue;
        if (qobject_cast<QIODevice*>(connection)){
            QIODevice* ioCon = qobject_cast<QIODevice*>(connection);
            if (!ioCon->isOpen()) return;
            if(ioCon->waitForReadyRead(1)){
                buff.append(ioCon->readAll());
            }
        }
        if (buff.isEmpty()) continue;
        if (protocol == "Ublox"){
            UbloxParser parser(connection);
            QMap<QString,QByteArray> mess = parser.parseMessage(&buff);
            if (mess.isEmpty()) continue;
            Message* decodedMessage = parser.decode(mess);
            if (dynamic_cast<NAV::SOL*>(decodedMessage)){
                NAV::SOL *info = dynamic_cast<NAV::SOL*>(decodedMessage);
                U4 iTOW = info->iTOW;
                U1 gpsFix = info->gpsFix;
                if (gpsFix == 0){
                    if (eventSettingsSolLost && (flagsMap[key]["RTK Sol lost"] == INDEX_FLAGS_FALSE || flagsMap[key]["RTK Sol lost"] == INDEX_FLAGS_UNKNOWN)){
                        QString localTime = QTime::currentTime().toString("hh:mm:ss.zzz");
                        QString event = key + ": Решение потеряно";
                        addItemToLogTable(localTime, QString::number(iTOW), event);
                    }
                    flagsMap[key]["RTK Sol lost"] = INDEX_FLAGS_TRUE;
                    flagsMap[key]["RTK Sol found"] = INDEX_FLAGS_FALSE;
                }
                else {
                    if (eventSettingsSolFound && (flagsMap[key]["RTK Sol found"] == INDEX_FLAGS_FALSE || flagsMap[key]["RTK Sol found"] == INDEX_FLAGS_UNKNOWN)){
                        QString localTime = QTime::currentTime().toString("hh:mm:ss.zzz");
                        QString event = key + ": Решение найдено";
                        addItemToLogTable(localTime, QString::number(iTOW), event);
                    }
                    flagsMap[key]["RTK Sol found"] = INDEX_FLAGS_TRUE;
                    flagsMap[key]["RTK Sol lost"] = INDEX_FLAGS_FALSE;
                }
            }
            else if (dynamic_cast<NAV::RELPOSNED*>(decodedMessage)){
                NAV::RELPOSNED *info = dynamic_cast<NAV::RELPOSNED*>(decodedMessage);
                U4 iTOW = info->iTOW;
                X4 flags = info->flags;
                U1 relSol = getBits(flags,3,4);
                if (relSol == 0){
                    if (eventSettingsRelSolLost && (flagsMap[key]["RTK Rel Sol lost"] == INDEX_FLAGS_FALSE || flagsMap[key]["RTK Rel Sol lost"] == INDEX_FLAGS_UNKNOWN)){
                        QString localTime = QTime::currentTime().toString("hh:mm:ss.zz");
                        QString event = key + ": Относительное решение потеряно";
                        addItemToLogTable(localTime, QString::number(iTOW), event);
                    }
                    flagsMap[key]["RTK Rel Sol lost"] = INDEX_FLAGS_TRUE;
                    flagsMap[key]["RTK Rel Sol found"] = INDEX_FLAGS_FALSE;
                }
                else {
                    if (eventSettingsRelSolFound && (flagsMap[key]["RTK Rel Sol found"] == INDEX_FLAGS_FALSE || flagsMap[key]["RTK Rel Sol found"] == INDEX_FLAGS_UNKNOWN)){
                        QString localTime = QTime::currentTime().toString("hh:mm:ss.zz");
                        QString event = key + ": Относительное решение найдено";
                        addItemToLogTable(localTime, QString::number(iTOW), event);
                    }
                    flagsMap[key]["RTK Rel Sol found"] = INDEX_FLAGS_TRUE;
                    flagsMap[key]["RTK Rel Sol lost"] = INDEX_FLAGS_FALSE;
                }
            }
        }
        else if (protocol == "Unicore"){
            UnicoreParser parser(connection);
            UnicoreMessage mess = parser.parseMessage(&buff);
            if (mess.data.isEmpty()) return;
            if (!mess.isAscii) return;

        }
    }
}


