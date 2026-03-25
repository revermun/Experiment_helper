#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "connectionSettings.h"
#include "notesdialog.h"
#include "startstopactionsdialog.h"
#include "dataandgraphsdialog.h"
#include "experimentconfigurationdialog.h"
#include "deviceconfigurationsdialog.h"

/// TODO: Работа с папкой эксперимента.
/// Необходимо при запуске программы сообщить пользователю о том, что нужно выбрать папку эксперимента
/// кнопки либо блокируются либо при нажатии выскакивает сообщение о том что нужно выбрать эксперимент
///
/// При изменении/удалении чего либо имеющего свой файл нужно "на ходу" изменить этот файл !!!! надо уточнить !!!!
/// TODO: Кнопка редактирования подключения
/// добавить проверки при редактировании и удалении из списка устройства того, что оно подключенно и если это так, то выводить ошибку
/// TODO: Кнопка удаления подключения
/// добавить проверки при редактировании и удалении из списка устройства того, что оно подключенно и если это так, то выводить ошибку
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
        QString baudrate =           parameters.at(INDEX_SERIAL_BAUDRATE);
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
    for (const auto &key: devicesMap.keys()) {
        QString deviceName = key;
        QPair<QString,QList<QString>> settings = devicesMap.value(key);
        QString protocolName = settings.first;
        QList<QString> parameters = settings.second;
        addItemToConnectionsTable(protocolName,settings.second);
    }
}

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
        if (!file.open(QIODevice::ReadOnly))
            return;
        QDomDocument doc("document");
        if (!doc.setContent(&file)) {
            file.close();
            return;
        }
        file.close();

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
                QString baudrate =           parametersXml.attribute("baudrate");
                QString dataBits =          parametersXml.attribute("data_bits");
                QString TCPPort =           parametersXml.attribute("TCP_port_number");
                QString parity =            parametersXml.attribute("parity");
                QString stopBits =          parametersXml.attribute("stop_bits");
                QString TCPCount =          parametersXml.attribute("TCP_connections_number");

                parameters << deviceName << deviceType << transferProtocol << baudrate << dataBits << TCPPort << parity << stopBits << TCPCount;

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
        QDir experimentDir(experimentDirectory);
        experimentDir.mkpath(experimentDirectory + '/' + "Configurations" + '/' + "Experiment_configurations");
        experimentDir.mkpath(experimentDirectory + '/' + "Configurations" + '/' + "Device_configurations");
        experimentDir.mkpath(experimentDirectory + '/' + "Configurations" + '/' + "Lap_presets");

        setupTableSize(ui->tableWidgetConnections);
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
                QString baudrate =           settings.second.at(INDEX_SERIAL_BAUDRATE);
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
            QString baudrate =   settings.second.at(INDEX_SERIAL_BAUDRATE);
            QString dataBits =  settings.second.at(INDEX_SERIAL_DATA_BITS);
            QString TCPPort =   settings.second.at(INDEX_SERIAL_TCP_PORT);
            QString parity =    settings.second.at(INDEX_SERIAL_PARITY);
            QString stopBits =  settings.second.at(INDEX_SERIAL_STOP_BITS);
            QString TCPCount =  settings.second.at(INDEX_SERIAL_TCP_COUNT);

            parameters << baudrate << dataBits << TCPPort << parity << stopBits << TCPCount;

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
            QString baudrate =   settings.second.at(INDEX_SERIAL_BAUDRATE);
            QString dataBits =  settings.second.at(INDEX_SERIAL_DATA_BITS);
            QString TCPPort =   settings.second.at(INDEX_SERIAL_TCP_PORT);
            QString parity =    settings.second.at(INDEX_SERIAL_PARITY);
            QString stopBits =  settings.second.at(INDEX_SERIAL_STOP_BITS);
            QString TCPCount =  settings.second.at(INDEX_SERIAL_TCP_COUNT);

            parameters << baudrate << dataBits << TCPPort << parity << stopBits << TCPCount;

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
    QDialog dialog(this);
    dialog.setWindowTitle("Настройка событий");
    dialog.setFixedSize(600, 250);

    QGridLayout *layout = new QGridLayout(&dialog);

    QLabel *labelTopLeft = new QLabel("Условия, являющиеся событиями:");
    QLabel *labelTopRight = new QLabel("Вкл/Выкл");
    QLabel *labelSolutionFound = new QLabel("Fixed решение RTK найдено");
    QLabel *labelSolutionLost = new QLabel("Решение потеряно");
    QLabel *labelETC = new QLabel("И т.д.");
    QCheckBox *chebBoxSolutionFound = new QCheckBox();
    QCheckBox *chebBoxSolutionLost = new QCheckBox();
    QCheckBox *chebBoxETC = new QCheckBox();
    QPushButton *okButton = new QPushButton("Готово");
    QPushButton *cancelButton = new QPushButton("Отмена");

    chebBoxSolutionFound->setChecked(eventSettingsSolutionFound);
    chebBoxSolutionLost->setChecked(eventSettingsSolutionLost);
    chebBoxETC->setChecked(eventSettingsETC);

    layout->addWidget(labelTopLeft, 0, 0);
    layout->addWidget(labelTopRight, 0, 1);
    layout->addWidget(labelSolutionFound, 1, 0);
    layout->addWidget(labelSolutionLost, 2, 0);
    layout->addWidget(labelETC, 3, 0);
    layout->addWidget(chebBoxSolutionFound, 1, 1);
    layout->addWidget(chebBoxSolutionLost, 2, 1);
    layout->addWidget(chebBoxETC, 3, 1);
    layout->addWidget(okButton, 4, 0);
    layout->addWidget(cancelButton, 4, 1);

    // Подключить сигнал кнопки к закрытию диалога
    QObject::connect(okButton, &QPushButton::clicked, &dialog, &QDialog::accept);
    QObject::connect(cancelButton, &QPushButton::clicked, &dialog, &QDialog::reject);

    // Модальное выполнение
    if (dialog.exec() == QDialog::Accepted) {
        eventSettingsSolutionFound = chebBoxSolutionFound->isChecked();
        eventSettingsSolutionLost = chebBoxSolutionLost->isChecked();
        eventSettingsETC = chebBoxETC->isChecked();
        qDebug() << eventSettingsSolutionFound << eventSettingsSolutionLost << eventSettingsETC;
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
    QSerialPort* connection = new QSerialPort(this);
    QString port;
    int baudrate;
    QString protocolName = ui->tableWidgetConnections->item(row,INDEX_CONN_TABLE_TYPE)->text();
    QTableWidgetItem *onnOffItem = new QTableWidgetItem;
    onnOffItem->setBackground(QBrush(QColor(255,255,0)));

    ui->tableWidgetConnections->setItem(row, INDEX_CONN_TABLE_ON_OFF, onnOffItem);
    if (protocolName == "Serial"){
        // bool parity = devicesMap[deviceName].second.at(INDEX_SERIAL_PARITY).toInt();
        // int dataBits = devicesMap[deviceName].second.at(INDEX_SERIAL_DATA_BITS).toInt();
        // int stopBits = devicesMap[deviceName].second.at(INDEX_SERIAL_STOP_BITS).toInt();
        // connection->setDataBits(QSerialPort::Data7);
        // connection->setStopBits(stopBits);
        port = ui->tableWidgetConnections->item(row,INDEX_CONN_TABLE_TCP_PORT)->text();
        baudrate = devicesMap[deviceName].second.at(INDEX_SERIAL_BAUDRATE).toInt();
    }
    dumpSimpleMap(devicesMap);
    qDebug() << port << baudrate;
    connection->setPortName(port);
    connection->setBaudRate(baudrate);

    // пробуем подключится
    if (!connection->open(QIODevice::ReadWrite)) {
        QMessageBox::warning(this, "Ошибка", "Не удалось подключится к порту");
        return;
    }
    connection->close();
    onnOffItem->setBackground(QBrush(QColor(0,255,0)));
    ui->tableWidgetConnections->setItem(row, INDEX_CONN_TABLE_ON_OFF, onnOffItem);
    connectionsMap.insert(deviceName, connection);
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
    QSerialPort* connection = connectionsMap[deviceName];
    connection->close();
    onnOffItem->setBackground(QBrush(QColor(255,0,0)));
    ui->tableWidgetConnections->setItem(row, INDEX_CONN_TABLE_ON_OFF, onnOffItem);
    connectionsMap.remove(deviceName);
}
