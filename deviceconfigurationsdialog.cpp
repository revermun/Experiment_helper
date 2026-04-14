#include "deviceconfigurationsdialog.h"
#include "ui_deviceconfigurationsdialog.h"
#include "enums.h"
#include "ubloxparser.h"
#include "convertors.h"
#include "unicoreparser.h"

/// протоколы unicore
/// отностельное решение:
///  при двух подключенных антенеах - HEADING
///  при одной подключенной антенне - RTK
/// ------------------------------------------------------------------------------
/// |                    НАЗВАНИЕ                       |     ТИП     |  СТАТУС  |
/// ------------------------------------------------------------------------------
/// |BESTNAV                                            |   MESSAGE   |     +    |
/// |GPSEPH                                             |   MESSAGE   |     +    |
/// |OBSVM                                              |   MESSAGE   |     +    |
/// |UNIHEADING                                         |   MESSAGE   |     +    |
/// |MASK                                               |   COMMAND   |     +    |
/// |CONFIG RTK                                         |   COMMAND   |     +    |
/// |CONFIG DPGS                                        |   COMMAND   |     +    |
/// |CONFIG ANTIJAM                                     |   COMMAND   |     -    |
/// |MODE ROVER/BASE/ROVER SURVEY/ROVER AUTOMOTIVE/UAV  |   COMMAND   |     +    |
/// ------------------------------------------------------------------------------

/// Общие настройки:
/// Настройка     передача                                          чтение
/// системы         MASK +                                              +
/// min elev        MASK +                                              +
/// CN0             MASK +                                              +
/// datum           BESTNAVA +                                          +
/// RTK             CONGFIG RTK +                                       +
/// antijam         CONFIG ANTIJAM +                                    +
/// eph             GPSEPHA/BD3EPHA/GLOEPHA/GALEPHA x +                 +
/// raw             OBSVMA x  +                                         +
/// vel pos         BESTNAVA x +                                        +
/// mode            MODE +                                              +
/// rel pos         UNIHEADINGA x +                                     +

/// протоколы ublox
/// ubx-cfg-dat - SET реализация в первой вкладке   |+
/// ubx-cfg-cfg - Command                           |+
/// ubx-cfg-dgnss - GET/SET                         |+
/// ubx-cfg-gnss - GET/SET                          |+ (может быть потребуются доработки)
/// ubx-cfg-msg - Poll Request                      |+
/// ubx-cfg-nav5 - Get/Set                          |+
/// ubx-cfg-prt - Poll Request                      |+
/// ubx-cfg-rate - GET/SET                          |+
/// ubx-cfg-rst - command                           |+
///
/// ubx-nav-clock - Periodic/Polled                 |+
/// ubx-nav-dop - Periodic/Polled                   |+
/// ubx-nav-orb - Periodic/Polled                   |+
/// ubx-nav-dgps - Periodic/Polled                  |+
/// ubx-nav-posecef - Periodic/Polled               |+
/// ubx-nav-posllh - Periodic/Polled                |+
/// ubx-nav-relposned - Periodic/Polled             |+
/// ubx-nav-sat - Periodic/Polled                   |+
/// ubx-nav-sol - Periodic/Polled                   |+
/// ubx-nav-status - Periodic/Polled                |+
/// ubx-nav-velecef - Periodic/Polled               |+
/// ubx-nav-velned - Periodic/Polled                |+
/// Итого надо сделать: 0 сообщений
///


///TODO: Основные настройки устройств
/// Что нужно?
/// Используемые диапазоны частот. Плюс, надо учитывать, какие диапазоны частот может принимать соответствующая антенна.
/// !!!!!!
/// непонятно
/// !!!!


deviceConfigurationsDialog::deviceConfigurationsDialog(QMap<QString,QPair<QString,QList<QString>>> devicesMap,QMap<QString, QSerialPort*> connectionsMap, QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::deviceConfigurationsDialog)
{
    ui->setupUi(this);
    this->devicesMap = devicesMap;
    this->connectionsMap = connectionsMap;
    qDebug() << connectionsMap;
    foreach (QString key, connectionsMap.keys()) {
        ui->comboBoxDevice1->addItem(key);
        ui->comboBoxDevice2->addItem(key);
    }
    ui->comboBoxDevice1->setCurrentIndex(-1);
    ui->comboBoxDevice2->setCurrentIndex(-1);

    ui->comboRecieverSystems->setAllCheckedText("Все системы выбраны");
    ui->comboRecieverSystems->setNoneCheckedText("Ни одна из систем не выбрана");

    QObject::connect(ui->comboBoxDevice1, SIGNAL(currentTextChanged(QString)), this, SLOT(deviceChangeEvent()));
    QObject::connect(ui->comboBoxDevice2, SIGNAL(currentTextChanged(QString)), this, SLOT(deviceChangeEvent()));

    foreach (auto object, ui->scrollAreaMessageSettingsContents->children()) {
        if(!qobject_cast<QVBoxLayout*>(object)){
            QWidget *widget = qobject_cast<QWidget*>(object);
            widget->setHidden(true);
        }
    }
    messagesIDMapUBX["CLOCK"] = QPair<uint8_t,uint8_t>(NAV::CLOCK::classID, NAV::CLOCK::messageID);
    messagesIDMapUBX["DGPS"] = QPair<uint8_t,uint8_t>(NAV::DGPS::classID, NAV::DGPS::messageID);
    messagesIDMapUBX["DOP"] = QPair<uint8_t,uint8_t>(NAV::DOP::classID, NAV::DOP::messageID);
    messagesIDMapUBX["ORB"] = QPair<uint8_t,uint8_t>(NAV::ORB::classID, NAV::ORB::messageID);
    messagesIDMapUBX["POSECEF"] = QPair<uint8_t,uint8_t>(NAV::POSECEF::classID, NAV::POSECEF::messageID);
    messagesIDMapUBX["POSLLH"] = QPair<uint8_t,uint8_t>(NAV::POSLLH::classID, NAV::POSLLH::messageID);
    messagesIDMapUBX["RELPOSNED"] = QPair<uint8_t,uint8_t>(NAV::RELPOSNED::classID, NAV::RELPOSNED::messageID);
    messagesIDMapUBX["SAT"] = QPair<uint8_t,uint8_t>(NAV::SAT::classID, NAV::SAT::messageID);
    messagesIDMapUBX["SOL"] = QPair<uint8_t,uint8_t>(NAV::SOL::classID, NAV::SOL::messageID);
    messagesIDMapUBX["STATUS"] = QPair<uint8_t,uint8_t>(NAV::STATUS::classID, NAV::STATUS::messageID);
    messagesIDMapUBX["VELECEF"] = QPair<uint8_t,uint8_t>(NAV::VELECEF::classID, NAV::VELECEF::messageID);
    messagesIDMapUBX["VELNED"] = QPair<uint8_t,uint8_t>(NAV::VELNED::classID, NAV::VELNED::messageID);

    messagesIDMapUBX["DGNSS"] = QPair<uint8_t,uint8_t>(CFG::DGNSS::classID, CFG::DGNSS::messageID);
    messagesIDMapUBX["CFG"] = QPair<uint8_t,uint8_t>(CFG::_CFG::classID, CFG::_CFG::messageID);
    messagesIDMapUBX["GNSS"] = QPair<uint8_t,uint8_t>(CFG::GNSS::classID, CFG::GNSS::messageID);
    messagesIDMapUBX["MSG"] = QPair<uint8_t,uint8_t>(CFG::MSG::classID, CFG::MSG::messageID);
    messagesIDMapUBX["DAT"] = QPair<uint8_t,uint8_t>(CFG::DAT::classID, CFG::DAT::messageID);
    messagesIDMapUBX["NAV5"] = QPair<uint8_t,uint8_t>(CFG::NAV5::classID, CFG::NAV5::messageID);
    messagesIDMapUBX["PRT"] = QPair<uint8_t,uint8_t>(CFG::PRT::classID, CFG::PRT::messageID);
    messagesIDMapUBX["RATE"] = QPair<uint8_t,uint8_t>(CFG::RATE::classID, CFG::RATE::messageID);
    messagesIDMapUBX["RST"] = QPair<uint8_t,uint8_t>(CFG::RST::classID, CFG::RST::messageID);
    messagesIDMapUBX["ITFM"] = QPair<uint8_t,uint8_t>(CFG::ITFM::classID, CFG::ITFM::messageID);

    messagesIDMapUBX["RAWX"] = QPair<uint8_t,uint8_t>(RXM::RAWX::classID, RXM::RAWX::messageID);
    messagesIDMapUBX["SFRBX"] = QPair<uint8_t,uint8_t>(RXM::SFRBX::classID, RXM::SFRBX::messageID);

    framesMap["NAV-CLOCK"] = ui->frameCLOCK;
    framesMap["NAV-DGPS"] = ui->frameDGPS;
    framesMap["NAV-DOP"] = ui->frameDOP;
    framesMap["NAV-ORB"] = ui->frameORB;
    framesMap["NAV-POSECEF"] = ui->framePOSECEF;
    framesMap["NAV-POSLLH"] = ui->framePOSLLH;
    framesMap["NAV-RELPOSNED"] = ui->frameRELPOSNED;
    framesMap["NAV-SAT"] = ui->frameSAT;
    framesMap["NAV-SOL"] = ui->frameSOL;
    framesMap["NAV-STATUS"] = ui->frameSTATUS;
    framesMap["NAV-VELECEF"] = ui->frameVELECEF;
    framesMap["NAV-VELNED"] = ui->frameVELNED;
    framesMap["CFG-DGNSS"] = ui->frameDGNSS;
    framesMap["CFG-GNSS"] = ui->frameGNSS;
    framesMap["CFG-CFG"] = ui->frameCFG;
    framesMap["CFG-MSG"] = ui->frameMSG;
    framesMap["CFG-DAT"] = ui->frameDAT;
    framesMap["CFG-NAV5"] = ui->frameNAV5;
    framesMap["CFG-PRT"] = ui->framePRT;
    framesMap["CFG-RATE"] = ui->frameRATE;
    framesMap["CFG-CLOCK"] = ui->frameCLOCK;
    framesMap["CFG-RST"] = ui->frameRST;
    framesMap["CFG-ITFM"] = ui->frameITFM;
    framesMap["BESTNAV"] = ui->frameBESTNAV;
    framesMap["OBSVM"] = ui->frameOBSVM;
    framesMap["GPSEPH"] = ui->frameGPSEPH;
    framesMap["UNIHEADING"] = ui->frameUNIHEADING;
    framesMap["MASK"] = ui->frameMASK;
    framesMap["CONFIG RTK"] = ui->frameCONFIGRTK;
    framesMap["CONFIG DGPS"] = ui->frameCONFIGDGPS;
    framesMap["CONFIG ANTIJAM"] = ui->frameCONFIGANTIJAM;
    framesMap["MODE"] = ui->frameMODE;


    messagesDescriptionsMap["NAV-CLOCK"]        = "Информация о состоянии часов приемника. Содержит данные о смещении времени и его точности.";
    messagesDescriptionsMap["NAV-DGPS"]         = "Статус дифференциальной коррекции (DGPS). Показывает, какие поправки применяются к решению.";
    messagesDescriptionsMap["NAV-DOP"]          = "Параметры геометрического фактора ухудшения точности (DOP). Оценка качества геометрии спутников.";
    messagesDescriptionsMap["NAV-ORB"]          = "Информация об орбитальных параметрах спутников (элементах эфемерид и альманаха).";
    messagesDescriptionsMap["NAV-POSECEF"]      = "Координаты местоположения в геоцентрической системе координат ECEF (Earth-Centered, Earth-Fixed).";
    messagesDescriptionsMap["NAV-POSLLH"]       = "Геодезические координаты (широта, долгота, высота). Самое популярное сообщение для получения позиции.";
    messagesDescriptionsMap["NAV-RELPOSNED"]    = "Относительное положение в локальной системе координат \"Север-Восток-Низ\" (NED). Используется в режимах RTK.";
    messagesDescriptionsMap["NAV-SAT"]          = "Детальная информация по каждому отслеживаемому спутнику. Заменяет устаревшее NAV-SVINFO.";
    messagesDescriptionsMap["NAV-SOL"]          = "Общее навигационное решение (положение, скорость, время и статус в одном сообщении).";
    messagesDescriptionsMap["NAV-STATUS"]       = "Текущий статус навигационного решения.";
    messagesDescriptionsMap["NAV-VELECEF"]      = "Вектор скорости в системе координат ECEF.";
    messagesDescriptionsMap["NAV-VELNED"]       = "Вектор скорости в локальной системе координат \"Север-Восток-Низ\" (NED).";
    messagesDescriptionsMap["CFG-DGNSS"]        = "Настройка параметров дифференциальной навигации (DGNSS).";
    messagesDescriptionsMap["CFG-GNSS"]         = "Настройка используемых спутниковых систем (GPS, ГЛОНАСС, Galileo и др.).";
    messagesDescriptionsMap["CFG-CFG"]          = "Управление конфигурацией (сохранение, загрузка, очистка).";
    messagesDescriptionsMap["CFG-MSG"]          = "Настройка выдачи сообщений (включение/выключение и частота).";
    messagesDescriptionsMap["CFG-DAT"]          = "Настройка геодезического датума для вывода координат.";
    messagesDescriptionsMap["CFG-NAV5"]         = "Основные настройки навигационного движка (параметры 2D/3D решения).";
    messagesDescriptionsMap["CFG-PRT"]          = "Настройка портов ввода/вывода (UART, USB, SPI).";
    messagesDescriptionsMap["CFG-RATE"]         = "Настройка частоты навигационных решений (периода измерений).";
    messagesDescriptionsMap["CFG-CLOCK"]        = "Управление состоянием опорного генератора.";
    messagesDescriptionsMap["CFG-RST"]          = "Команда для сброса приемника (горячий, теплый, холодный старт).";
    messagesDescriptionsMap["CFG-ITFM"]         = "Настройка системы обнаружения и подавления помех.";
    messagesDescriptionsMap["BESTNAV"]          = "Содержит лучшее решение о местоположении, времени и скорости, включая данные о валидности и точности";
    messagesDescriptionsMap["GPSEPH"]           = "Выводит эфемериды (орбитальные параметры) спутников GPS, используемые для вычисления их координат.";
    messagesDescriptionsMap["OBSVM"]            = "Содержит сырые наблюдения (псевдодальности, фазы несущей, SNR).";
    messagesDescriptionsMap["UNIHEADING"]       = "Вычисляет курс (угол направления) между двумя антеннами, подключенными к приемнику.";
    messagesDescriptionsMap["MASK"]             = "Настраивает маски: маску возвышения спутника (угол над горизонтом), маску С/Ш (порог сигнала) и временную маску.";
    messagesDescriptionsMap["CONFIG RTK"]       = "Настраивает режим RTK (Real-Time Kinematic): выбор базовой станции, режим фиксированного решения и т.д.";
    messagesDescriptionsMap["CONFIG DGPS"]      = "Настраивает параметры дифференциальной коррекции (DGPS), включая источники коррекции и пороги.";
    messagesDescriptionsMap["CONFIG ANTIJAM"]   = "Управляет настройками системы анти-джамминга (подавления помех).";
    messagesDescriptionsMap["MODE"]             = "Переключает рабочий режим приемника (например, одиночный, RTK, статический).";

    QByteArray buffArray;
    buffArray.resize(2);
    buffArray[0] = NAV::CLOCK::classID;
    buffArray[1] = NAV::CLOCK::messageID;
    messagesNamesMap.insert(buffArray,"CLOCK");
    buffArray[0] = NAV::DOP::classID;
    buffArray[1] = NAV::DOP::messageID;
    messagesNamesMap.insert(buffArray,"DOP");
    buffArray[0] = NAV::ORB::classID;
    buffArray[1] = NAV::ORB::messageID;
    messagesNamesMap.insert(buffArray,"ORB");
    buffArray[0] = NAV::DGPS::classID;
    buffArray[1] = NAV::DGPS::messageID;
    messagesNamesMap.insert(buffArray,"DGPS");
    buffArray[0] = NAV::POSECEF::classID;
    buffArray[1] = NAV::POSECEF::messageID;
    messagesNamesMap.insert(buffArray,"POSECEF");
    buffArray[0] = NAV::POSLLH::classID;
    buffArray[1] = NAV::POSLLH::messageID;
    messagesNamesMap.insert(buffArray,"POSLLH");
    buffArray[0] = NAV::RELPOSNED::classID;
    buffArray[1] = NAV::RELPOSNED::messageID;
    messagesNamesMap.insert(buffArray,"RELPOSNED");
    buffArray[0] = NAV::SAT::classID;
    buffArray[1] = NAV::SAT::messageID;
    messagesNamesMap.insert(buffArray,"SAT");
    buffArray[0] = NAV::SOL::classID;
    buffArray[1] = NAV::SOL::messageID;
    messagesNamesMap.insert(buffArray,"SOL");
    buffArray[0] = NAV::STATUS::classID;
    buffArray[1] = NAV::STATUS::messageID;
    messagesNamesMap.insert(buffArray,"STATUS");
    buffArray[0] = NAV::VELECEF::classID;
    buffArray[1] = NAV::VELECEF::messageID;
    messagesNamesMap.insert(buffArray,"VELECEF");
    buffArray[0] = NAV::VELNED::classID;
    buffArray[1] = NAV::VELNED::messageID;
    messagesNamesMap.insert(buffArray,"VELNED");
    buffArray[0] = RXM::RAWX::classID;
    buffArray[1] = RXM::RAWX::messageID;
    messagesNamesMap.insert(buffArray,"RAWX");
    buffArray[0] = RXM::SFRBX::classID;
    buffArray[1] = RXM::SFRBX::messageID;
    messagesNamesMap.insert(buffArray,"SFRBX");

    QStringList reciverSystems = {"GPS L1C/A", "GPS L2C",
                                  "SBAS L1C/A",
                                  "Galileo E1", "Galileo E5b",
                                  "BeiDou B1I", "BeiDou B2I",
                                  "IMES L1",
                                  "QZSS L1C/A", "QZSS L1S", "QZSS L2C",
                                  "GLONASS L1", "GLONASS L2"};

    foreach (QString system, reciverSystems) {
        ui->comboRecieverSystems->addCheckItem(system,Qt::CheckState::Unchecked,Qt::CheckState::Unchecked);
    }
    QTimer *timer = new QTimer(this);
    connect(timer, SIGNAL(timeout()), this, SLOT(parseMessage()));
    timer->start(10);

}

deviceConfigurationsDialog::~deviceConfigurationsDialog()
{
    delete ui;
}

void deviceConfigurationsDialog::MASKcheckClickEvent(bool state)
{
    QCheckBox* check = qobject_cast<QCheckBox*>(sender());
    QMap<QCheckBox*, QPair<QSpinBox*, QObjectList>> systemsMap;
    QPair<QSpinBox*, QObjectList> GPSpair;
    GPSpair.first = ui->spinMASKGPS;
    QObjectList gpsList = {ui->checkMASKGPSl1ca, ui->checkMASKGPSl2c};
    GPSpair.second = gpsList;
    QPair<QSpinBox*, QObjectList> BDSpair;
    BDSpair.first = ui->spinMASKBDS;
    QObjectList BDSList = {ui->checkMASKBDSb1, ui->checkMASKBDSb2};
    BDSpair.second = BDSList;
    QPair<QSpinBox*, QObjectList> GLOpair;
    GLOpair.first = ui->spinMASKGLO;
    QObjectList GLOList = {ui->checkMASKGLOr1, ui->checkMASKGLOr2};
    GLOpair.second = GLOList;
    QPair<QSpinBox*, QObjectList> GALpair;
    GALpair.first = ui->spinMASKGPS;
    QObjectList GALList = {ui->checkMASKGALe1, ui->checkMASKGALe5b};
    GALpair.second = GALList;
    QPair<QSpinBox*, QObjectList> QZSSpair;
    QZSSpair.first = ui->spinMASKGPS;
    QObjectList QZSSList = {ui->checkMASKQZSSq1ca, ui->checkMASKQZSSq2c};
    QZSSpair.second = QZSSList;
    systemsMap[ui->checkMASKGPS] = GPSpair;
    systemsMap[ui->checkMASKBDS] = BDSpair;
    systemsMap[ui->checkMASKGLO] = GLOpair;
    systemsMap[ui->checkMASKGAL] = GALpair;
    systemsMap[ui->checkMASKQZSS] = QZSSpair;
    if (state){
        foreach (QObject* obj, systemsMap[check].second) {
            QCheckBox* check = qobject_cast<QCheckBox*>(obj);
            check->setChecked(true);
            check->setEnabled(false);
        }
    }
    else{
        foreach (QObject* obj, systemsMap[check].second) {
            QCheckBox* check = qobject_cast<QCheckBox*>(obj);
            check->setChecked(false);
            check->setEnabled(true);
        }
    }
}

void deviceConfigurationsDialog::comboCONFIGRTKchangeEvent(QString text)
{
    ui->frameCONFIGRTKparams->setEnabled(text != "DISABLE");
}

void deviceConfigurationsDialog::deviceChangeEvent()
{
    QComboBox* combo = qobject_cast<QComboBox*>(sender());
    if (combo->currentIndex() == -1 || ui->comboBoxDevice1->currentText() == ui->comboBoxDevice2->currentText()){
        return;
    }
    streamBuffer.clear();
    foreach (QString key, connectionsMap.keys()) {
        if (key == combo->currentText()) {
            currentConnection = connectionsMap[key];
            // currentConnection->open(QSerialPort::ReadWrite);
        }
        // else connectionsMap[key]->close();
    }

    // connectionsMap[combo->currentText()]->open(QSerialPort::ReadWrite);

    QPair<QString,QList<QString>> deviceInfo = devicesMap[combo->currentText()];
    if (ui->comboBoxDevice1 == combo) ui->comboBoxDevice2->setCurrentText(ui->comboBoxDevice1->currentText());
    else ui->comboBoxDevice1->setCurrentText(ui->comboBoxDevice2->currentText());
    QString type = deviceInfo.second.at(INDEX_GENERAL_DEVICE_TYPE);
    ui->labelType->setText("Тип: " + type);
    QList<QWidget*> tab2Widgets = {ui->pushButtonLoadConfig, ui->pushButtonSaveConfig, ui->pushButtonSet,
                                    ui->pushButtonPoll,  ui->pushButtonSaveInDevice};
    foreach (QWidget* widget, tab2Widgets) {
        widget->setEnabled(true);
    }
    QString protocol = deviceInfo.second.at(INDEX_GENERAL_PROTOCOL);
    this->protocol = protocol;
    ui->treeWidgetMessages->clear();
    if (protocol == "Ublox"){
        QTreeWidgetItem* configMessagesItem = new QTreeWidgetItem();
        configMessagesItem->setText(0,"Конфигурационные сообщения");
        QTreeWidgetItem* cfgDatItem = new QTreeWidgetItem();
        QTreeWidgetItem* cfgCfgItem = new QTreeWidgetItem();
        QTreeWidgetItem* cfgDgnssItem = new QTreeWidgetItem();
        QTreeWidgetItem* cfgGnssItem = new QTreeWidgetItem();
        QTreeWidgetItem* cfgMsgItem = new QTreeWidgetItem();
        QTreeWidgetItem* cfgNav5Item = new QTreeWidgetItem();
        QTreeWidgetItem* cfgPrtItem = new QTreeWidgetItem();
        QTreeWidgetItem* cfgRateItem = new QTreeWidgetItem();
        QTreeWidgetItem* cfgRstItem = new QTreeWidgetItem();
        QTreeWidgetItem* cfgItfmItem = new QTreeWidgetItem();
        cfgDatItem->setText(0,"CFG-DAT");
        cfgCfgItem->setText(0,"CFG-CFG");
        cfgDgnssItem->setText(0,"CFG-DGNSS");
        cfgGnssItem->setText(0,"CFG-GNSS");
        cfgMsgItem->setText(0,"CFG-MSG");
        cfgNav5Item->setText(0,"CFG-NAV5");
        cfgPrtItem->setText(0,"CFG-PRT");
        cfgRateItem->setText(0,"CFG-RATE");
        cfgRstItem->setText(0,"CFG-RST");
        cfgItfmItem->setText(0,"CFG-ITFM");
        QList<QTreeWidgetItem*> configMessages =  {cfgDatItem, cfgCfgItem, cfgDgnssItem, cfgGnssItem, cfgMsgItem, cfgNav5Item, cfgPrtItem, cfgRateItem, cfgRstItem, cfgItfmItem};
        configMessagesItem->addChildren(configMessages);

        QTreeWidgetItem* navMessagesItem = new QTreeWidgetItem();
        navMessagesItem->setText(0,"Навигационные сообщения");
        QTreeWidgetItem* navClockItem       = new QTreeWidgetItem();
        QTreeWidgetItem* navDopItem         = new QTreeWidgetItem();
        QTreeWidgetItem* navOrbItem         = new QTreeWidgetItem();
        QTreeWidgetItem* navDgnssItem       = new QTreeWidgetItem();
        QTreeWidgetItem* navPosecefItem     = new QTreeWidgetItem();
        QTreeWidgetItem* navPosellhItem     = new QTreeWidgetItem();
        QTreeWidgetItem* navRelposnedItem   = new QTreeWidgetItem();
        QTreeWidgetItem* navSatItem         = new QTreeWidgetItem();
        QTreeWidgetItem* navSolItem         = new QTreeWidgetItem();
        QTreeWidgetItem* navStatusItem      = new QTreeWidgetItem();
        QTreeWidgetItem* navVelecefItem     = new QTreeWidgetItem();
        QTreeWidgetItem* navVelnedItem      = new QTreeWidgetItem();
        navClockItem->setText(0,    "NAV-CLOCK");
        navDopItem->setText(0,      "NAV-DOP");
        navOrbItem->setText(0,      "NAV-ORB");
        navDgnssItem->setText(0,    "NAV-DGPS");
        navPosecefItem->setText(0,  "NAV-POSECEF");
        navPosellhItem->setText(0,  "NAV-POSLLH");
        navRelposnedItem->setText(0,"NAV-RELPOSNED");
        navSatItem->setText(0,      "NAV-SAT");
        navSolItem->setText(0,      "NAV-SOL");
        navStatusItem->setText(0,   "NAV-STATUS");
        navVelecefItem->setText(0,  "NAV-VELECEF");
        navVelnedItem->setText(0,   "NAV-VELNED");

        QList<QTreeWidgetItem*> navMessages =  {navClockItem, navDopItem, navOrbItem, navDgnssItem, navPosecefItem, navPosellhItem,
                                                navRelposnedItem, navSatItem, navSolItem, navStatusItem, navVelecefItem, navVelnedItem};
        navMessagesItem->addChildren(navMessages);

        QList<QTreeWidgetItem*> messageTypes = {configMessagesItem, navMessagesItem};
        ui->treeWidgetMessages->insertTopLevelItems(0, messageTypes);
        ui->labelVelUnits->setText("см/с");
    }
    else if (protocol == "Unicore"){
        QTreeWidgetItem* configMessagesItem = new QTreeWidgetItem();
        configMessagesItem->setText(0,"Конфигурационные сообщения");
        QTreeWidgetItem* MASKItem           = new QTreeWidgetItem();
        QTreeWidgetItem* CONFIGRTKItem      = new QTreeWidgetItem();
        QTreeWidgetItem* CONFIGDGNSSItem    = new QTreeWidgetItem();
        QTreeWidgetItem* CONFIGANTIJAMItem  = new QTreeWidgetItem();
        QTreeWidgetItem* MODEItem           = new QTreeWidgetItem();
        MASKItem->setText(0,"MASK");
        CONFIGRTKItem->setText(0,"CONFIG RTK");
        CONFIGDGNSSItem->setText(0,"CONFIG DGPS");
        CONFIGANTIJAMItem->setText(0,"CONFIG ANTIJAM");
        MODEItem->setText(0,"MODE");
        QList<QTreeWidgetItem*> configMessages =  {MASKItem, CONFIGRTKItem, CONFIGDGNSSItem, CONFIGANTIJAMItem, MODEItem};
        configMessagesItem->addChildren(configMessages);

        QTreeWidgetItem* navMessagesItem = new QTreeWidgetItem();
        navMessagesItem->setText(0,"Навигационные сообщения");
        QTreeWidgetItem* BESTNAVItem    = new QTreeWidgetItem();
        QTreeWidgetItem* GPSEPHItem     = new QTreeWidgetItem();
        QTreeWidgetItem* OBSVMItem      = new QTreeWidgetItem();
        QTreeWidgetItem* UNIHEADINGItem = new QTreeWidgetItem();
        BESTNAVItem->setText(0,    "BESTNAV");
        GPSEPHItem->setText(0,     "GPSEPH");
        OBSVMItem->setText(0,      "OBSVM");
        UNIHEADINGItem->setText(0, "UNIHEADING");

        QList<QTreeWidgetItem*> navMessages =  {BESTNAVItem, GPSEPHItem, OBSVMItem, UNIHEADINGItem};
        navMessagesItem->addChildren(navMessages);

        QList<QTreeWidgetItem*> messageTypes = {configMessagesItem, navMessagesItem};
        ui->treeWidgetMessages->insertTopLevelItems(0, messageTypes);
        ui->labelVelUnits->setText("м/с");
    }
    ui->pushButtonSend->setEnabled(false);
}

void deviceConfigurationsDialog::tabChangeEvent()
{
    QList<QWidget*> tab2Widgets = {ui->pushButtonLoadConfig, ui->pushButtonSaveConfig, ui->pushButtonSet,
                                    ui->pushButtonPoll,  ui->pushButtonSaveInDevice};
    if(ui->tabWidget->currentIndex() == 1 && ui->comboBoxDevice2->currentText().isEmpty()){
        foreach (QWidget* widget, tab2Widgets) {
            widget->setEnabled(false);
        }
    }
}

void deviceConfigurationsDialog::enableITFMSettings(bool state)
{
    QList<QWidget*> settingsWidgets = {ui->spinITFMbbThresh, ui->spinITFMcwThresh, ui->comboITFMantType};
    foreach (QWidget* widget, settingsWidgets) {
        widget->setEnabled(state);
    }
}

void deviceConfigurationsDialog::enablePosFrame(bool state)
{
    setChildrenEnabled(ui->framePos, state);
    setChildrenEnabled(ui->framePosRates, state);
    setChildrenEnabled(ui->groupBoxPos, state);
}
void deviceConfigurationsDialog::enableVelFrame(bool state)
{
    setChildrenEnabled(ui->frameVel, state);
    setChildrenEnabled(ui->frameVelRates, state);
    setChildrenEnabled(ui->groupBoxVel, state);
}
void deviceConfigurationsDialog::enableRelposFrame(bool state)
{
    setChildrenEnabled(ui->frameRelpos, state);
}
void deviceConfigurationsDialog::enableEphFrame(bool state)
{
    setChildrenEnabled(ui->frameEph, state);
}
void deviceConfigurationsDialog::enableRawFrame(bool state)
{
    setChildrenEnabled(ui->frameRaw, state);
}

void deviceConfigurationsDialog::setupTableSize(QTableWidget* table) {
    // Автоматическая подгонка столбцов
    table->resizeColumnsToContents();
    table->resizeRowsToContents();

    // Отключаем скроллбары (если таблица небольшая)
    table->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    table->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

    // Вычисляем и устанавливаем размер
    table->setFixedSize(
        table->horizontalHeader()->length() + table->verticalHeader()->width() + table->frameWidth() * 2,
        table->verticalHeader()->length() + table->horizontalHeader()->height() + table->frameWidth() * 2
        );
}

void deviceConfigurationsDialog::enableNAV5AltLines(QString text){
    if(text == "1 - 2D only"){
        ui->spinNAV5AltVar->setEnabled(true);
        ui->spinNAV5FixAlt->setEnabled(true);
    }
    else{
        ui->spinNAV5AltVar->setEnabled(false);
        ui->spinNAV5FixAlt->setEnabled(false);
    }
}

void deviceConfigurationsDialog::showPortSettings(QString text){
    if (!text.contains("UART")){
        setChildrenHidden(ui->framePRTUART, true);
    }
    else setChildrenHidden(ui->framePRTUART, false);
}


void deviceConfigurationsDialog::parseMessage()
{
    if (currentConnection == nullptr){
        return;
    }
    if (!currentConnection->isOpen()){
        return;
    }

    if(currentConnection->waitForReadyRead(1)){
        streamBuffer.append(currentConnection->readAll());
    }
    if (protocol == "Ublox"){
        UbloxParser parser(currentConnection);
        QMap<QString,QByteArray> mess = parser.parseMessage(&streamBuffer);
        if (mess.isEmpty()) return;
        if (mess["byteClass"].at(0) == ACK::_ACK::classID && mess["byteID"].at(0) == ACK::_ACK::messageID){
            qDebug() << "ACK-ACK";
        }
        else if (mess["byteClass"].at(0) == ACK::NAK::classID && mess["byteID"].at(0) == ACK::NAK::messageID){
            qDebug() << "ACK-NAK";
        }

        Message* decodedMessage = parser.decode(mess);
        if (dynamic_cast<CFG::MSG::MSGRATES*>(decodedMessage)){
            CFG::MSG::MSGRATES *info = dynamic_cast<CFG::MSG::MSGRATES*>(decodedMessage);
            U1 msgClass = info->msgClass;
            U1 msgID = info->msgID;
            U1 rate1 = info->rate1;
            U1 rate2 = info->rate2;
            U1 rate3 = info->rate3;
            U1 rate4 = info->rate4;
            U1 rate5 = info->rate5;
            // U1 rate6 = info->rate6;
            QMap<QByteArray,QString> map = messagesNamesMap;
            QByteArray key;
            key.resize(2);
            key[0] = msgClass;
            key[1] = msgID;
            QString messName = map[key];
            if (ui->tabWidget->currentIndex() == 1) ui->comboMSG->setCurrentText(messName);
            ui->spinMSGI2C->setValue(rate1);
            ui->spinMSGUART1->setValue(rate2);
            ui->spinMSGUART2->setValue(rate3);
            ui->spinMSGUSB->setValue(rate4);
            ui->spinMSGSPI->setValue(rate5);
            if (messName == "SFRBX"){
                ui->checkRecieverEph->setChecked((rate1 + rate2 + rate3 + rate4 + rate5) > 0);
                ui->spinRecieverEph1->setValue(rate1);
                ui->spinRecieverEph2->setValue(rate2);
                ui->spinRecieverEph3->setValue(rate3);
                ui->spinRecieverEph4->setValue(rate4);
                ui->spinRecieverEph5->setValue(rate5);

            }
            if (messName == "RAWX"){
                ui->checkRecieverRaw->setChecked((rate1 + rate2 + rate3 + rate4 + rate5) > 0);
                ui->spinRecieverRaw1->setValue(rate1);
                ui->spinRecieverRaw2->setValue(rate2);
                ui->spinRecieverRaw3->setValue(rate3);
                ui->spinRecieverRaw4->setValue(rate4);
                ui->spinRecieverRaw5->setValue(rate5);
            }
            if (messName == "VELNED" || messName == "VELECEF"){
                if ((rate1 + rate2 + rate3 + rate4 + rate5) > 0) velCheck = true;
            }
            if (messName == "VELNED"){
                if ((rate1 + rate2 + rate3 + rate4 + rate5) > 0){
                    ui->checkRecieverVelNED->setChecked(true);
                    ui->spinRecieverVel1->setValue(rate1);
                    ui->spinRecieverVel2->setValue(rate2);
                    ui->spinRecieverVel3->setValue(rate3);
                    ui->spinRecieverVel4->setValue(rate4);
                    ui->spinRecieverVel5->setValue(rate5);
                }
                else {ui->checkRecieverVelNED->setChecked(false); velCheck = false;}
            }
            if (messName == "VELECEF"){
                if ((rate1 + rate2 + rate3 + rate4 + rate5) > 0){
                    ui->checkRecieverVelECEF->setChecked(true);
                    ui->spinRecieverVel1->setValue(rate1);
                    ui->spinRecieverVel2->setValue(rate2);
                    ui->spinRecieverVel3->setValue(rate3);
                    ui->spinRecieverVel4->setValue(rate4);
                    ui->spinRecieverVel5->setValue(rate5);
                }
                else {ui->checkRecieverVelECEF->setChecked(false);
                    if (!velCheck){
                        ui->spinRecieverVel1->setValue(rate1);
                        ui->spinRecieverVel2->setValue(rate2);
                        ui->spinRecieverVel3->setValue(rate3);
                        ui->spinRecieverVel4->setValue(rate4);
                        ui->spinRecieverVel5->setValue(rate5);
                    }

                }
                ui->checkRecieverVel->setChecked(velCheck);

            }
            if (messName == "POSLLH" || messName == "POSECEF"){
                if ((rate1 + rate2 + rate3 + rate4 + rate5) > 0) posCheck = true;
            }
            if (messName == "POSLLH"){
                if ((rate1 + rate2 + rate3 + rate4 + rate5) > 0){
                    ui->checkRecieverPosLLH->setChecked(true);
                    ui->spinRecieverPos1->setValue(rate1);
                    ui->spinRecieverPos2->setValue(rate2);
                    ui->spinRecieverPos3->setValue(rate3);
                    ui->spinRecieverPos4->setValue(rate4);
                    ui->spinRecieverPos5->setValue(rate5);
                }
                else {ui->checkRecieverPosLLH->setChecked(false); posCheck = false;}
            }
            if (messName == "POSECEF"){
                if ((rate1 + rate2 + rate3 + rate4 + rate5) > 0){
                    ui->checkRecieverPosECEF->setChecked(true);
                    ui->spinRecieverPos1->setValue(rate1);
                    ui->spinRecieverPos2->setValue(rate2);
                    ui->spinRecieverPos3->setValue(rate3);
                    ui->spinRecieverPos4->setValue(rate4);
                    ui->spinRecieverPos5->setValue(rate5);
                }
                else {ui->checkRecieverPosECEF->setChecked(false);
                    if (!posCheck){
                        ui->spinRecieverPos1->setValue(rate1);
                        ui->spinRecieverPos2->setValue(rate2);
                        ui->spinRecieverPos3->setValue(rate3);
                        ui->spinRecieverPos4->setValue(rate4);
                        ui->spinRecieverPos5->setValue(rate5);
                    }
                }
                ui->checkRecieverPos->setChecked(posCheck);
            }
            if (messName == "RELPOSNED"){
                ui->checkRecieverRelpos->setChecked((rate1 + rate2 + rate3 + rate4 + rate5) > 0);
                ui->spinRecieverRelPos1->setValue(rate1);
                ui->spinRecieverRelPos2->setValue(rate2);
                ui->spinRecieverRelPos3->setValue(rate3);
                ui->spinRecieverRelPos4->setValue(rate4);
                ui->spinRecieverRelPos5->setValue(rate5);
            }
        }
        else if (dynamic_cast<CFG::DGNSS*>(decodedMessage)){
            CFG::DGNSS *info = dynamic_cast<CFG::DGNSS*>(decodedMessage);
            U1 dgnssMode = info->dgnssMode;
            int index;
            if (dgnssMode == 2){
                index = 0;
            }
            else index = 1;
            ui->comboDGNSS->setCurrentIndex(index);
            ui->checkRecieverRTKSolution->setChecked(index);
        }
        else if (dynamic_cast<CFG::DAT::GET*>(decodedMessage)){
            CFG::DAT::GET *info = dynamic_cast<CFG::DAT::GET*>(decodedMessage);
            U2 datumNum = info->datumNum;
            U4 datumName1 = info->datumName1;
            U2 datumName2 = info->datumName2;
            QByteArray name;
            QDataStream stream(&name, QIODevice::WriteOnly);
            stream.setByteOrder(QDataStream::LittleEndian);
            stream << datumName1 << datumName2;
            qDebug() << name;
            R8 majA = info->majA;
            R8 flat = info->flat;
            R4 dX = info->dX;
            R4 dY = info->dY;
            R4 dZ = info->dZ;
            R4 rotX = info->rotX;
            R4 rotY = info->rotY;
            R4 rotZ = info->rotZ;
            R4 scale = info->scale;
            ui->labelDATName->setText(name);
            ui->labelDATID->setText(QString::number(datumNum));
            ui->labelDATMajA->setText(QString::number(majA,'f',4));
            ui->labelDATFlatt->setText(QString::number(flat,'f',10));
            ui->labelDATXShift->setText(QString::number(dX,'f',3));
            ui->labelDATYShift->setText(QString::number(dY,'f',3));
            ui->labelDATZShift->setText(QString::number(dZ,'f',3));
            ui->labelDATXRot->setText(QString::number(rotX,'f',3));
            ui->labelDATYRot->setText(QString::number(rotY,'f',3));
            ui->labelDATZRot->setText(QString::number(rotZ,'f',3));
            ui->labelDATScale->setText(QString::number(scale,'f',3));
            ui->lineRecieverDatumId->setText(name);
        }
        else if (dynamic_cast<CFG::RATE*>(decodedMessage)){
            CFG::RATE *info = dynamic_cast<CFG::RATE*>(decodedMessage);
            U2 measRate = info->measRate;
            U2 navRate = info->navRate;
            U2 timeRef = info->timeRef;
            ui->spinRATEPer->setValue(measRate);
            ui->spinRATE->setValue(navRate);
            ui->comboRATE->setCurrentIndex(timeRef);
        }
        else if (dynamic_cast<CFG::PRT::USB*>(decodedMessage)){
            CFG::PRT::USB *info = dynamic_cast<CFG::PRT::USB*>(decodedMessage);
            // U1 reserved1 = info->reserved1;
            X2 txReady = info->txReady;
            // U4 reserved2 = info->reserved2;
            // U4 reserved3 = info->reserved3;
            X2 inProtoMask = info->inProtoMask;
            X2 outProtoMask = info->outProtoMask;
            U4 reserved4 = info->reserved4;
            bool en = getBit(txReady,0);
            bool pol = getBit(txReady,1);
            bool txTimeout = getBit(reserved4,17);
            U1 pin = getBits(txReady,2,6);
            U2 threshhold = getBits(txReady,7,15);
            for(int i = 0; i<ui->comboPRTIn->count(); i++){
                if (ui->comboPRTIn->itemText(i).section(' ',0,0).toUInt() == inProtoMask){
                    ui->comboPRTIn->setCurrentIndex(i);
                    break;
                }
            }
            for(int i = 0; i<ui->comboPRTOut->count(); i++){
                if (ui->comboPRTOut->itemText(i).section(' ',0,0).toUInt() == outProtoMask){
                    ui->comboPRTOut->setCurrentIndex(i);
                    break;
                }
            }
            ui->checkTXEn->setChecked(en);
            ui->checkTXPol->setChecked(pol);
            ui->spinTXPin->setValue(pin);
            ui->checkTXTimeout->setChecked(txTimeout);
            ui->spinTXThreshhold->setValue(threshhold*8);
        }
        else if (dynamic_cast<CFG::PRT::UART*>(decodedMessage)){
            CFG::PRT::UART *info = dynamic_cast<CFG::PRT::UART*>(decodedMessage);
            // U1 portID = info->portID;
            // U1 reserved1 = info->reserved1;
            X2 txReady = info->txReady;
            X4 mode = info->mode;
            U4 baudRate = info->baudRate;
            X2 inProtoMask = info->inProtoMask;
            X2 outProtoMask = info->outProtoMask;
            X2 flags = info->flags;
            // U2 reserved2 = info->reserved2;
            bool en = getBit(txReady,0);
            bool pol = getBit(txReady,1);
            bool txTimeout = getBit(flags,1);
            U1 pin = getBits(txReady,2,6);
            U2 threshhold = getBits(txReady,7,15);
            for(int i = 0; i<ui->comboPRTIn->count(); i++){
                if (ui->comboPRTIn->itemText(i).section(' ',0,0).toUInt() == inProtoMask){
                    ui->comboPRTIn->setCurrentIndex(i);
                    break;
                }
            }
            for(int i = 0; i<ui->comboPRTOut->count(); i++){
                if (ui->comboPRTOut->itemText(i).section(' ',0,0).toUInt() == outProtoMask){
                    ui->comboPRTOut->setCurrentIndex(i);
                    break;
                }
            }
            ui->checkTXEn->setChecked(en);
            ui->checkTXPol->setChecked(pol);
            ui->spinTXPin->setValue(pin);
            ui->checkTXTimeout->setChecked(txTimeout);
            ui->spinTXThreshhold->setValue(threshhold*8);
            U1 charLen = getBits(mode, 6, 7);
            U1 parity = getBits(mode, 9, 11);
            if(parity > 1) parity = 2;
            U1 nStopBits = getBits(mode, 12, 13);
            ui->comboUARTDataBits->setCurrentIndex(charLen);
            ui->comboUARTParity->setCurrentIndex(parity);
            ui->comboUARTStopBits->setCurrentIndex(nStopBits);
            ui->comboUARTBaud->setCurrentText(QString::number(baudRate));
        }
        else if (dynamic_cast<CFG::GNSS*>(decodedMessage)){
            CFG::GNSS *info = dynamic_cast<CFG::GNSS*>(decodedMessage);
            // U1 msgVer = info->msgVer;
            U1 numTrkChHw = info->numTrkChHw;
            U1 numTrkChUse = info->numTrkChUse;
            // U1 numConfigBlocks = info->numConfigBlocks;
            ui->lineGNSSMaxCh->setText(QString::number(numTrkChHw));
            ui->spinGNSSUseCh->setValue(numTrkChUse);
            foreach (CFG::GNSS::Repeated svData, info->repeatedList) {
                U1 gnssId = svData.gnssId;
                U1 resTrkCh = svData.resTrkCh;
                U1 maxTrkCh = svData.maxTrkCh;
                // U1 reserved = svData.reserved;
                X4 flags = svData.flags;
                bool enabled = getBit(flags,0);
                U1 sigCfgMask = getBits(flags,16,23);
                QGridLayout* layout = qobject_cast<QGridLayout*>(ui->frameGNSS->layout());
                int row = gnssId + 1;
                qobject_cast<QCheckBox*>(layout->itemAtPosition(row,2)->widget())->setChecked(true);
                qobject_cast<QCheckBox*>(layout->itemAtPosition(row,3)->widget())->setChecked(enabled);
                qobject_cast<QSpinBox*>(layout->itemAtPosition(row,4)->widget())->setValue(resTrkCh);
                qobject_cast<QSpinBox*>(layout->itemAtPosition(row,5)->widget())->setValue(maxTrkCh);
                bool L1C;
                bool L2C;
                bool E1;
                bool E5;
                bool B1;
                bool B2;
                bool L1;
                bool L1S;
                bool L2;
                // "GPS L1C/A", "GPS L2C",
                //     "SBAS L1C/A",
                //     "Galileo E1", "Galileo E5b",
                //     "BeiDou B1I", "BeiDou B2I",
                //     "IMES L1",
                //     "QZSS L1C/A", "QZSS L1S", "QZSS L2C",
                //     "GLONASS L1", "GLONASS L2"
                switch (gnssId) {
                case 0x00:
                    L1C = getBit(sigCfgMask,0);
                    L2C = getBit(sigCfgMask,4);
                    ui->checkGNSSGPSSigL1C->setChecked(L1C);
                    ui->checkGNSSGPSSigL2C->setChecked(L2C);
                    ui->comboRecieverSystems->setitemCheckState(0, L1C);
                    ui->comboRecieverSystems->setitemCheckState(1, L2C);
                    break;
                case 0x01:
                    L1C = getBit(sigCfgMask,0);
                    ui->checkGNSSSBASSigL1C->setChecked(L1C);
                    ui->comboRecieverSystems->setitemCheckState(2, L1C);
                    break;
                case 0x02:
                    E1 = getBit(sigCfgMask,0);
                    E5 = getBit(sigCfgMask,5);
                    ui->checkGNSSGalSigE1->setChecked(E1);
                    ui->checkGNSSGalSigE5->setChecked(E5);
                    ui->comboRecieverSystems->setitemCheckState(3, E1);
                    ui->comboRecieverSystems->setitemCheckState(4, E5);
                    break;
                case 0x03:
                    B1 = getBit(sigCfgMask,0);
                    B2 = getBit(sigCfgMask,4);
                    ui->checkGNSSBeiSigB1->setChecked(B1);
                    ui->checkGNSSBeiSigB2->setChecked(B2);
                    ui->comboRecieverSystems->setitemCheckState(5, B1);
                    ui->comboRecieverSystems->setitemCheckState(6, B2);
                    break;
                case 0x04:
                    L1 = getBit(sigCfgMask,0);
                    ui->checkGNSSIMESSigL1C->setChecked(L1);
                    ui->comboRecieverSystems->setitemCheckState(7, L1);

                    break;
                case 0x05:
                    L1C = getBit(sigCfgMask,0);
                    L1S = getBit(sigCfgMask,2);
                    L2C = getBit(sigCfgMask,4);
                    ui->checkGNSSQZSSSigL1C->setChecked(L1C);
                    ui->checkGNSSQZSSSigL1S->setChecked(L1S);
                    ui->checkGNSSQZSSSigL2C->setChecked(L2C);
                    ui->comboRecieverSystems->setitemCheckState(8, L1C);
                    ui->comboRecieverSystems->setitemCheckState(9, L1S);
                    ui->comboRecieverSystems->setitemCheckState(10, L2C);
                    break;
                case 0x06:
                    L1 = getBit(sigCfgMask,0);
                    L2 = getBit(sigCfgMask,4);
                    ui->checkGNSSGLOSigL1->setChecked(L1);
                    ui->checkGNSSGLOSigL2->setChecked(L2);
                    ui->comboRecieverSystems->setitemCheckState(11, L1);
                    ui->comboRecieverSystems->setitemCheckState(12, L2);
                    break;
                default:
                    break;
                }
            }
        }
        else if (dynamic_cast<CFG::NAV5*>(decodedMessage)){
            CFG::NAV5 *info = dynamic_cast<CFG::NAV5*>(decodedMessage);
            // X2 mask = info->mask;
            U1 dynModel = info->dynModel;
            U1 fixMode = info->fixMode;
            I4 fixedAlt = info->fixedAlt;
            U4 fixedAltVar = info->fixedAltVar;
            I1 minElev = info->minElev;
            U1 drLimit = info->drLimit;
            U2 pDop = info->pDop;
            U2 tDop = info->tDop;
            U2 pAcc = info->pAcc;
            U2 tAcc = info->tAcc;
            U1 staticHoldThresh = info->staticHoldThresh;
            U1 dgnssTimeout = info->dgnssTimeout;
            U1 cnoThreshNumSVs = info->cnoThreshNumSVs;
            U1 cnoThresh = info->cnoThresh;
            // U1 reserved1 = info->reserved1;
            // U1 reserved2 = info->reserved2;
            U2 staticHoldMaxDist = info->staticHoldMaxDist;
            U1 utcStandard = info->utcStandard;
            // U1 reserved3 = info->reserved3;
            // U1 reserved4 = info->reserved4;
            // U1 reserved5 = info->reserved5;
            // U1 reserved6 = info->reserved6;
            // U1 reserved7 = info->reserved7;
            int dynModeIndex =  dynModel == 0? 0: dynModel-1;
            int fixModeIndex = fixMode - 1;
            int UTCIndex =  utcStandard == 0? 0:
                            utcStandard == 3? 1:
                            utcStandard == 5? 2:
                            utcStandard == 6? 3:
                            utcStandard == 7? 4:0;
            ui->spinNAV5FixAlt->setValue((double)fixedAlt/100);
            ui->spinNAV5AltVar->setValue((double)fixedAltVar/10000);
            ui->spinNAV5MinElev->setValue(minElev);
            ui->spinNAV5DRLimit->setValue(drLimit);
            ui->spinNAV5PDOP->setValue((double)pDop/10);
            ui->spinNAV5TDOP->setValue((double)tDop/10);
            ui->spinNAV5PAcc->setValue(pAcc);
            ui->spinNAV5TAcc->setValue(tAcc);
            ui->spinNAV5SHT->setValue((double)staticHoldThresh/100);
            ui->spinNAV5DGNSSTimeout->setValue(dgnssTimeout);
            ui->spinNAV5SVs->setValue(cnoThreshNumSVs);
            ui->spinNAV5CNOThresh->setValue(cnoThresh);
            ui->spinNAV5MaxDist->setValue(staticHoldMaxDist);
            ui->comboNAV5DynModel->setCurrentIndex(dynModeIndex);
            ui->comboNAV5FixMode->setCurrentIndex(fixModeIndex);
            ui->comboNAV5UTC->setCurrentIndex(UTCIndex);
            ui->spinRecieverMinElev->setValue(minElev);
            ui->spinRecieverCNOThresh->setValue(cnoThresh);
            QString modelName = ui->comboNAV5DynModel->currentText().section(" - ",1);
            QList<QString> itemList;
            for (int i = 0; i < ui->comboRecieverMode->count(); ++i) {
                itemList.append(ui->comboRecieverMode->itemText(i));
            }
            bool flag = 0;
            foreach (QString item, itemList) {
                if (modelName == item){
                    ui->comboRecieverMode->setCurrentText(modelName);
                    flag = 1;
                    break;
                }
            }
            if (!flag){
                ui->comboRecieverMode->setCurrentText("Other");
            }
        }
        else if (dynamic_cast<CFG::ITFM*>(decodedMessage)){
            CFG::ITFM *info = dynamic_cast<CFG::ITFM*>(decodedMessage);
            X4 config = info->config;
            X4 config2 = info->config2;
            bool en = getBit(config,31);
            U1 bbThresh = getBits(config,0,3);
            U1 cwThresh = getBits(config,4,8);
            U1 antSettings = getBits(config2,12,13);
            ui->checkITFMEn->setChecked(en);
            ui->spinITFMbbThresh->setValue(bbThresh);
            ui->spinITFMcwThresh->setValue(cwThresh);
            ui->comboITFMantType->setCurrentIndex(antSettings);
            ui->checkRecieverJamming->setChecked(en);
        }
        else if (dynamic_cast<NAV::POSECEF*>(decodedMessage)){
            NAV::POSECEF *info = dynamic_cast<NAV::POSECEF*>(decodedMessage);
            U4 iTOW = info->iTOW;
            I4 ecefX = info->ecefX;
            I4 ecefY = info->ecefY;
            I4 ecefZ = info->ecefZ;
            U4 pAcc = info->pAcc;
            ui->labelPOSECEFITOW->setText(QString::number((double)iTOW/1000, 'f', 3) + " s");
            ui->labelPOSECEFECEFX->setText(QString::number((double)ecefX/100, 'f', 2) + " м");
            ui->labelPOSECEFECEFY->setText(QString::number((double)ecefY/100, 'f', 2) + " м");
            ui->labelPOSECEFECEFZ->setText(QString::number((double)ecefZ/100, 'f', 2) + " м");
            ui->labelPOSECEFPOS3D->setText(QString::number((double)pAcc/100, 'f', 2) + " m");
        }
        else if (dynamic_cast<NAV::CLOCK*>(decodedMessage)){
            NAV::CLOCK *info = dynamic_cast<NAV::CLOCK*>(decodedMessage);
            U4 iTOW = info->iTOW;
            I4 clkB = info->clkB;
            I4 clkD = info->clkD;
            U4 tAcc = info->tAcc;
            U4 fAcc = info->fAcc;
            ui->labelCLOCKITOW->setText(QString::number((double)iTOW/1000, 'f', 3) + " s");
            ui->labelCLOCKB->setText(QString::number((double)clkB/1000, 'f', 3) + " us");
            ui->labelCLOCKD->setText(QString::number((double)clkD/1000, 'f', 3) + " us/s");
            ui->labelCLOCKT->setText(QString::number((double)tAcc/1000, 'f', 3) + " us");
            ui->labelCLOCKF->setText(QString::number((double)fAcc/1000000, 'f', 6) + " ppm");
        }
        else if (dynamic_cast<NAV::DOP*>(decodedMessage)){
            NAV::DOP *info = dynamic_cast<NAV::DOP*>(decodedMessage);
            U4 iTOW = info->iTOW;
            U2 gDOP = info->gDOP;
            U2 pDOP = info->pDOP;
            U2 tDOP = info->tDOP;
            U2 vDOP = info->vDOP;
            U2 hDOP = info->hDOP;
            U2 nDOP = info->nDOP;
            U2 eDOP = info->eDOP;
            ui->labelDOPITOW->setText(QString::number((double)iTOW/1000, 'f', 3) + " s");
            ui->labelDOPG->setText(QString::number((double)gDOP/100, 'f', 2));
            ui->labelDOPP->setText(QString::number((double)pDOP/100, 'f', 2));
            ui->labelDOPT->setText(QString::number((double)tDOP/100, 'f', 2));
            ui->labelDOPV->setText(QString::number((double)vDOP/100, 'f', 2));
            ui->labelDOPH->setText(QString::number((double)hDOP/100, 'f', 2));
            ui->labelDOPN->setText(QString::number((double)nDOP/100, 'f', 2));
            ui->labelDOPE->setText(QString::number((double)eDOP/100, 'f', 2));
        }
        else if (dynamic_cast<NAV::POSLLH*>(decodedMessage)){
            NAV::POSLLH *info = dynamic_cast<NAV::POSLLH*>(decodedMessage);
            U4 iTOW = info->iTOW;
            I4 lon = info->lon;
            I4 lat = info->lat;
            I4 height = info->height;
            I4 hMSL = info->hMSL;
            U4 hAcc = info->hAcc;
            U4 vAcc = info->vAcc;
            ui->labelPOSLLHITOW->setText(QString::number((double)iTOW/1000, 'f', 3) + " s");
            ui->labelPOSLLHLon->setText(QString::number((double)lon/10000000, 'f', 7) + " degrees");
            ui->labelPOSLLHLat->setText(QString::number((double)lat/10000000, 'f', 7) + " degrees");
            ui->labelPOSLLHHeight->setText(QString::number((double)height/1000, 'f', 3) + " m");
            ui->labelPOSLLHMSL->setText(QString::number((double)hMSL/1000, 'f', 3) + " m");
            ui->labelPOSLLHH->setText(QString::number((double)hAcc/1000, 'f', 3) + " m");
            ui->labelPOSLLHV->setText(QString::number((double)vAcc/1000, 'f', 3) + " m");
        }
        else if (dynamic_cast<NAV::RELPOSNED*>(decodedMessage)){
            NAV::RELPOSNED *info = dynamic_cast<NAV::RELPOSNED*>(decodedMessage);
            // U1 version = info->version;
            // U1 reserved1 = info->reserved1;
            U2 refStationId = info->refStationId;
            U4 iTOW = info->iTOW;
            I4 relPosN = info->relPosN;
            I4 relPosE = info->relPosE;
            I4 relPosD = info->relPosD;
            I4 relPosLength = info->relPosLength;
            I4 relPosHeading = info->relPosHeading;
            // U1 reserved2 = info->reserved2;
            // U1 reserved3 = info->reserved3;
            // U1 reserved4 = info->reserved4;
            // U1 reserved5 = info->reserved5;
            I1 relPosHPN = info->relPosHPN;
            I1 relPosHPE = info->relPosHPE;
            I1 relPosHPD = info->relPosHPD;
            I1 relPosHPLength = info->relPosHPLength;
            // U4 accN = info->accN;
            // U4 accE = info->accE;
            // U4 accD = info->accD;
            // U4 accLength = info->accLength;
            // U4 accHeading = info->accHeading;
            // U1 reserved6 = info->reserved6;
            // U1 reserved7 = info->reserved7;
            // U1 reserved8 = info->reserved8;
            // U1 reserved9 = info->reserved9;
            X4 flags = info->flags;
            bool gnnsFixOk = getBit(flags, 0);
            bool diffSoln = getBit(flags, 1);
            bool relPosValid = getBit(flags,2);
            int carSoln = (int)getBit(flags, 3) + ((int)getBit(flags, 4) << 1);
            bool isMoving = getBit(flags, 5);
            // bool refPosMiss = getBit(flags, 6);
            // bool refObsMiss = getBit(flags, 7);
            bool relPosHeadingValid = getBit(flags, 8);
            bool relPosNormalized = getBit(flags, 9);
            QString carSol = carSoln == 0? "no carrier phase range solution":
                             carSoln == 1? "carrier phase range solution with doubleing ambiguities"
                                          :"carrier phase range solution with fixed ambiguities";
            ui->checkRELPOSNEDFIXok->setChecked(gnnsFixOk);
            ui->checkRELPOSNEDDifSol->setChecked(diffSoln);
            ui->checkRELPOSNEDIsMoving->setChecked(isMoving);
            ui->checkRELPOSNEDNorm->setChecked(relPosNormalized);
            ui->checkRELPOSNEDHeading->setChecked(relPosHeadingValid);
            ui->checkRELPOSNEDPosVal->setChecked(relPosValid);
            ui->labelRELPOSNEDStatus->setText(carSol);
            ui->labelRELPOSNEDITOW->setText(QString::number((double)iTOW/1000, 'f', 3) + " s");
            ui->labelRELPOSNEDD->setText(QString::number((double)relPosD/100, 'f', 2) + " m");
            ui->labelRELPOSNEDN->setText(QString::number((double)relPosN/100, 'f', 2) + " m");
            ui->labelRELPOSNEDE->setText(QString::number((double)relPosE/100, 'f', 2) + " m");
            ui->labelRELPOSNEDHPD->setText(QString::number((double)relPosHPD/10000, 'f', 4) + " m");
            ui->labelRELPOSNEDHPN->setText(QString::number((double)relPosHPN/10000, 'f', 4) + " m");
            ui->labelRELPOSNEDHPE->setText(QString::number((double)relPosHPE/10000, 'f', 4) + " m");
            ui->labelRELPOSNEDL->setText(QString::number((double)relPosLength/100, 'f', 2) + " m");
            ui->labelRELPOSNEDH->setText(QString::number((double)relPosHeading/100000, 'f', 5) + " degrees");
            ui->labelRELPOSNEDHPL->setText(QString::number((double)relPosHPLength/10000, 'f', 4) + " m");
            ui->labelRELPOSNEDID->setText(QString::number(refStationId));

        }
        else if (dynamic_cast<NAV::SOL*>(decodedMessage)){
            NAV::SOL *info = dynamic_cast<NAV::SOL*>(decodedMessage);
            U4 iTOW = info->iTOW;
            // I4 fTOW = info->fTOW;
            I2 week = info->week;
            U1 gpsFix = info->gpsFix;
            X1 flags = info->flags;
            I4 ecefX = info->ecefX;
            I4 ecefY = info->ecefY;
            I4 ecefZ = info->ecefZ;
            U4 pAcc = info->pAcc;
            I4 ecefVX = info->ecefVX;
            I4 ecefVY = info->ecefVY;
            I4 ecefVZ = info->ecefVZ;
            U4 sAcc = info->sAcc;
            U2 pDOP = info->pDOP;
            // U1 reserved1 = info->reserved1;
            U1 numSV = info->numSV;
            // U4 reserved = info->reserved;
            // qDebug() << iTOW << fTOW << week << gpsFix << flags << ecefX << ecefY << ecefZ << pAcc << ecefVX << ecefVY << ecefVZ << sAcc << pDOP << reserved1 << numSV << reserved;
            QString fixType = gpsFix == 0? "No Fix":
                              gpsFix == 1? "Dead Reckoning only":
                              gpsFix == 2? "2D-Fix":
                              gpsFix == 3? "3D-Fix":
                              gpsFix == 4? "GPS + dead reckoning combined":
                              gpsFix == 5? "Time only fix": "reserved";
            QString GPSfixOK = getBit(flags,0)? "GPSfixOK ":"";
            QString DiffSoln = getBit(flags,1)? "DiffSoln ":"";
            QString WKNSET = getBit(flags,2)? "WKNSET ":"";
            QString TOWSET = getBit(flags,3)? "TOWSET ":"";
            QTableWidgetItem *item;
            item = new QTableWidgetItem(QString::number((double)week) + ":" + QString::number((double)iTOW/1000, 'f', 3) + " s"); ui->tableSOL->setItem(0,1,item);
            item = new QTableWidgetItem(fixType); ui->tableSOL->setItem(1,1,item);
            item = new QTableWidgetItem(GPSfixOK + DiffSoln + WKNSET + TOWSET); ui->tableSOL->setItem(2,1,item);
            item = new QTableWidgetItem("(" + QString::number((double)ecefX/100, 'f', 2) + "," + QString::number((double)ecefY/100, 'f', 2) + "," + QString::number((double)ecefZ/100, 'f', 2) + ")"); ui->tableSOL->setItem(3,1,item);
            item = new QTableWidgetItem(QString::number((double)pAcc)); ui->tableSOL->setItem(4,1,item);
            item = new QTableWidgetItem("(" + QString::number((double)ecefVX/100, 'f', 2) + "," + QString::number((double)ecefVY/100, 'f', 2) + "," + QString::number((double)ecefVZ/100, 'f', 2) + ")");  ui->tableSOL->setItem(5,1,item);
            item = new QTableWidgetItem(QString::number((double)sAcc/100, 'f', 3) + " m/s") ;  ui->tableSOL->setItem(6,1,item);
            item = new QTableWidgetItem(QString::number((double)pDOP/100));  ui->tableSOL->setItem(7,1,item);
            item = new QTableWidgetItem(QString::number((double)numSV));  ui->tableSOL->setItem(8,1,item);
            setupTableSize(ui->tableSOL);
        }
        else if (dynamic_cast<NAV::STATUS*>(decodedMessage)){
            NAV::STATUS *info = dynamic_cast<NAV::STATUS*>(decodedMessage);
            U4 iTOW = info->iTOW;
            U1 gpsFix = info->gpsFix;
            X1 flags = info->flags;
            X1 fixStat = info->fixStat;
            X1 flags2 = info->flags2;
            U4 ttff = info->ttff;
            U4 msss = info->msss;
            ui->labelSTATUSITOW->setText(QString::number((double)iTOW/1000) + " s");
            QString gpsFixstr = gpsFix == 0x00? "no fix":
                                gpsFix == 0x01? "dead reckoning only":
                                gpsFix == 0x02? "2D-fix":
                                gpsFix == 0x03? "3D-fix":
                                gpsFix == 0x04? "GPS + dead reckoning combined":
                                gpsFix == 0x05? "Time only fix":
                                                "reserved";
            bool gpsFixOK = getBit(flags,0);
            bool diffSoln = getBit(flags,1);
            bool wknSet = getBit(flags,2);
            bool towSet = getBit(flags,3);
            bool diffCor = getBit(fixStat,0);
            uint8_t mapMatching = (uint8_t)getBit(fixStat, 6) + ((uint8_t)getBit(fixStat, 7) << 1);
            QString mapMatchingStr = mapMatching == 0x00? "none":
                                     mapMatching == 0x01? "valid but not used":
                                     mapMatching == 0x02? "valid and used":
                                                          "valid and used, map matching data was applied";
            uint8_t psmState = (uint8_t)getBit(flags2, 0) + ((uint8_t)getBit(flags2, 1) << 1);
            QString psmStateStr = psmState == 0x00? "ACQUISITION":
                                  psmState == 0x01? "TRACKING":
                                  psmState == 0x02? "POWER OPTIMIZED TRACKING":
                                                    "INACTIVE";
            uint8_t spoofDetState = (uint8_t)getBit(flags2, 3) + ((uint8_t)getBit(flags2, 4) << 1);
            QString spoofDetStateStr = spoofDetState == 0x00? "Unknown or deactivated":
                                       spoofDetState == 0x01? "No spoofing indicated":
                                       spoofDetState == 0x02? "Spoofing indicated":
                                                              "Multiple spoofing indications";
            QTableWidgetItem *item;
            item = new QTableWidgetItem(gpsFixstr); ui->tableSTATUS->setItem(0,1,item);
            item = new QTableWidgetItem(gpsFixOK); ui->tableSTATUS->setItem(1,1,item);
            item = new QTableWidgetItem(diffSoln); ui->tableSTATUS->setItem(2,1,item);
            item = new QTableWidgetItem(wknSet); ui->tableSTATUS->setItem(3,1,item);
            item = new QTableWidgetItem(towSet); ui->tableSTATUS->setItem(4,1,item);
            item = new QTableWidgetItem(diffCor); ui->tableSTATUS->setItem(5,1,item);
            item = new QTableWidgetItem(mapMatchingStr); ui->tableSTATUS->setItem(6,1,item);
            item = new QTableWidgetItem(QString::number((double)ttff/1000, 'f',3)); ui->tableSTATUS->setItem(7,1,item);
            item = new QTableWidgetItem(QString::number((double)msss/1000, 'f',3)); ui->tableSTATUS->setItem(8,1,item);
            item = new QTableWidgetItem(psmStateStr); ui->tableSTATUS->setItem(9,1,item);
            item = new QTableWidgetItem(spoofDetStateStr); ui->tableSTATUS->setItem(10,1,item);
            setupTableSize(ui->tableSTATUS);
        }
        else if (dynamic_cast<NAV::VELECEF*>(decodedMessage)){
            NAV::VELECEF *info = dynamic_cast<NAV::VELECEF*>(decodedMessage);
            U4 iTOW = info->iTOW;
            I4 ecefVX = info->ecefVX;
            I4 ecefVY = info->ecefVY;
            I4 ecefVZ = info->ecefVZ;
            U4 sAcc = info->sAcc;
            ui->labelVELECEFITOW->setText(QString::number((double)iTOW/1000, 'f', 3) + " s");
            ui->labelVELECEFX->setText(QString::number((double)ecefVX/100, 'f', 2));
            ui->labelVELECEFY->setText(QString::number((double)ecefVY/100, 'f', 2));
            ui->labelVELECEFZ->setText(QString::number((double)ecefVZ/100, 'f', 2));
            ui->labelVELECEFVel->setText(QString::number((double)sAcc/100, 'f', 2));
        }
        else if (dynamic_cast<NAV::VELNED*>(decodedMessage)){
            NAV::VELNED *info = dynamic_cast<NAV::VELNED*>(decodedMessage);
            U4 iTOW = info->iTOW;
            I4 velN = info->velN;
            I4 velE = info->velE;
            I4 velD = info->velD;
            U4 speed = info->speed;
            U4 gSpeed = info->gSpeed;
            I4 heading = info->heading;
            U4 sAcc = info->sAcc;
            U4 cAcc = info->cAcc;
            ui->labelVELNEDITOW->setText(QString::number((double)iTOW/1000, 'f', 3) + " s");
            ui->labelVELNEDNorth->setText(QString::number((double)velN/100, 'f', 2) + " m/s");
            ui->labelVELNEDEast->setText(QString::number((double)velE/100, 'f', 2) + " m/s");
            ui->labelVELNEDDown->setText(QString::number((double)velD/100, 'f', 2) + " m/s");
            ui->labelVELNEDNorth->setText(QString::number((double)velN/100, 'f', 2) + " m/s");
            ui->labelVELNEDSpeed3D->setText(QString::number((double)speed/100, 'f', 2) + " m/s");
            ui->labelVELNEDGroundSpeed->setText(QString::number((double)gSpeed/100, 'f', 2) + " m/s");
            ui->labelVELNEDHeading->setText(QString::number((double)heading/100000, 'f', 2) + " degrees");
            ui->labelVELNEDVel->setText(QString::number((double)sAcc/100, 'f', 2) + " m/s");
            ui->labelVELNEDHead->setText(QString::number((double)cAcc/100000, 'f', 5) + " degrees");
        }
        else if (dynamic_cast<NAV::ORB*>(decodedMessage)){
            NAV::ORB *info = dynamic_cast<NAV::ORB*>(decodedMessage);
            // U4 iTOW = info->iTOW;
            // U1 version = info->version;
            // U1 numSv = info->numSv;
            // U1 reserved1 = info->reserved1;
            // U1 reserved2 = info->reserved2;
            int i = 0;
            ui->tableORB->clearContents();
            ui->tableORB->setRowCount(0);
            foreach (NAV::ORB::Repeated svData, info->repeatedList) {
                // U1 gnssId = svData->gnssId;
                U1 svId = svData.svId;
                X1 svFlag = svData.svFlag;
                X1 eph = svData.eph;
                X1 alm = svData.alm;
                X1 otherOrb = svData.otherOrb;
                U1 health = (U1)getBits(svFlag,0,1);
                U1 visibility = (U1)getBits(svFlag,2,3);
                U1 ephUsability = (U1)getBits(eph,0,4);
                U1 ephSource = (U1)getBits(eph,5,7);
                U1 almUsability = (U1)getBits(alm,0,4);
                U1 almSource = (U1)getBits(alm,5,7);
                U1 anoAopUsability = (U1)getBits(otherOrb,0,4);
                U1 type = (U1)getBits(otherOrb,5,7);
                QString healthStr = health == 0x00? "unknown":
                                    health == 0x01? "healthy":
                                    health == 0x02? "not healty":"";
                QString visibilityStr = visibility == 0x00? "unknown":
                                        visibility == 0x01? "below horizon":
                                        visibility == 0x02? "above horizon":
                                        visibility == 0x03? "above elevation mask":"";
                QString ephUsabilityStr = ephUsability == 31? "The usability period is unknown":
                                          ephUsability == 30? "The usability period is more than 450 minutes":
                                          ephUsability == 0? "Ephemeris can no longer be used":
                                                             "The usability period is between " + QString::number((ephUsability-1)*15) + " and " + QString::number((ephUsability)*15) + " minutes";
                QString ephSourceStr = ephSource == 0x00? "not available":
                                       ephSource == 0x01? "GNSS transmission":
                                       ephSource == 0x02? "external aiding":
                                                          "other";
                QString almUsabilityStr = almUsability == 31? "The usability period is unknown":
                                          almUsability == 30? "The usability period is more than 30 days":
                                          almUsability == 0?  "Almanac can no longer be used":
                                                             "The usability period is between " + QString::number((almUsability-1)) + " and " + QString::number((almUsability)) + " days";
                QString almSourceStr = almSource == 0x00? "not available":
                                       almSource == 0x01? "GNSS transmission":
                                       almSource == 0x02? "external aiding":
                                                          "other";
                QString anoAopUsabilityStr = anoAopUsability == 31? "The usability period is unknown":
                                             anoAopUsability == 30? "The usability period is more than 30 days":
                                             anoAopUsability == 0?  "Almanac can no longer be used":
                                                                    "The usability period is between " + QString::number((anoAopUsability-1)) + " and " + QString::number((anoAopUsability)) + " days";
                QString typeStr = type == 0x00? "not available":
                                  type == 0x01? "GNSS transmission":
                                  type == 0x02? "external aiding":
                                                     "other";

                ui->tableORB->insertRow(i);
                QTableWidgetItem *item;
                item = new QTableWidgetItem(svId); ui->tableORB->setItem(i,0,item);
                item = new QTableWidgetItem(ephUsabilityStr + "\n" + ephSourceStr); ui->tableORB->setItem(i,1,item);
                item = new QTableWidgetItem(almUsabilityStr + "\n" + almSourceStr); ui->tableORB->setItem(i,2,item);
                item = new QTableWidgetItem(anoAopUsabilityStr + "\n" + typeStr); ui->tableORB->setItem(i,3,item);
                item = new QTableWidgetItem(healthStr); ui->tableORB->setItem(i,4,item);
                item = new QTableWidgetItem(visibilityStr); ui->tableORB->setItem(i,5,item);
                i++;
            }
            setupTableSize(ui->tableORB);
        }
        else if (dynamic_cast<NAV::DGPS*>(decodedMessage)){
            ui->tableDGPS->clearContents();
            ui->tableDGPS->setRowCount(0);
            NAV::DGPS *info = dynamic_cast<NAV::DGPS*>(decodedMessage);
            U4 iTOW = info->iTOW;
            I4 age = info->age;
            I2 baseId = info->baseId;
            I2 baseHealth = info->baseHealth;
            // U1 numCh = info->numCh;
            U1 status = info->status;
            // U1 reserved1 = info->reserved1;
            // U1 reserved2 = info->reserved2;
            ui->labelDGPSITOW->setText(QString::number((double)iTOW/1000, 'f', 3) + " s");
            ui->labelDGPSAge->setText(QString::number((double)age/1000, 'f', 3) + " s");
            ui->labelDGPSId->setText(QString::number(baseId));
            ui->labelDGPSBaseHealth->setText(QString::number(baseHealth));
            QString statusStr = status == 0x01? "PR+PRR correction": "none";
            ui->labelDGPSStatus->setText(statusStr);
            int i = 0;
            foreach (NAV::DGPS::Repeated chData, info->repeatedList) {
                U1 svId = chData.svId;
                X1 flags = chData.flags;
                U2 ageC = chData.ageC;
                R4 prc = chData.prc;
                R4 prrc = chData.prrc;
                bool GPSUsed = getBit(flags,4);
                U1 channel = getBits(flags,0,3);
                ui->tableDGPS->insertRow(i);
                QTableWidgetItem *item;
                item = new QTableWidgetItem(svId); ui->tableDGPS->setItem(i,0,item);
                item = new QTableWidgetItem(GPSUsed); ui->tableDGPS->setItem(i,1,item);
                item = new QTableWidgetItem(channel); ui->tableDGPS->setItem(i,2,item);
                item = new QTableWidgetItem(QString::number((double)ageC/1000, 'f', 3)); ui->tableDGPS->setItem(i,3,item);
                item = new QTableWidgetItem(prc); ui->tableDGPS->setItem(i,4,item);
                item = new QTableWidgetItem(prrc); ui->tableDGPS->setItem(i,5,item);
                i++;
            }
            setupTableSize(ui->tableDGPS);
        }
        else if (dynamic_cast<NAV::SAT*>(decodedMessage)){
            ui->tableSAT->clearContents();
            ui->tableSAT->setRowCount(0);
            NAV::SAT *info = dynamic_cast<NAV::SAT*>(decodedMessage);
            U4 iTOW = info->iTOW;
            // U1 version = info->version;
            // U1 numSvs = info->numSvs;
            // U1 reserved1 = info->reserved1;
            // U1 reserved2 = info->reserved2;
            ui->labelSATITOW->setText(QString::number((double)iTOW/1000, 'f', 3));
            int i = 0;
            foreach (NAV::SAT::Repeated chData, info->repeatedList) {
                U1 gnssId = chData.gnssId;
                U1 svId = chData.svId;
                U1 cno = chData.cno;
                I1 elev = chData.elev;
                I2 azim = chData.azim;
                I2 prRes = chData.prRes;
                X4 flags = chData.flags;

                U1 qualityInd = getBits(flags,0,2);
                bool svUsed = getBit(flags,3);
                U1 health = getBits(flags,4,5);
                bool diffCor = getBit(flags,6);
                bool smothed = getBit(flags,7);
                U1 orbitSource = getBits(flags,8,10);
                bool eph = getBit(flags,11);
                bool alm = getBit(flags,12);
                bool ano = getBit(flags,13);
                bool aop = getBit(flags,14);
                QString elevStr = abs(elev)<=90? QString::number(elev):"unknown";
                QString azimStr = abs(elev)<=180? QString::number(azim):"unknown";
                QString qualityIndStr =     qualityInd == 0? "no signal":
                                            qualityInd == 1? "searching signal":
                                            qualityInd == 2? "signal aquired":
                                            qualityInd == 3? "signal detected but unusable":
                                            qualityInd == 4? "code locked and time synchronized":
                                            qualityInd == 5 || qualityInd == 6 || qualityInd == 7? "code and carrier locked and time synchronized": "unknown";
                QString healthStr = health == 1? "healthy":
                                    health == 2? "unhealthy": "unknown";
                QString orbitSourceStr =    orbitSource == 0? "no orbit information":
                                            orbitSource == 1? "ephemeris is used":
                                            orbitSource == 2? "almanac is used":
                                            orbitSource == 3? "AssistNow Offline orbit is used":
                                            orbitSource == 4? "AssistNow Autonomous orbit is used":
                                            orbitSource == 5 || orbitSource == 6 || orbitSource == 7? "other orbit information is used": "unknown";
                ui->tableSAT->insertRow(i);
                QTableWidgetItem *item;
                item = new QTableWidgetItem(gnssId); ui->tableSAT->setItem(i,0,item);
                item = new QTableWidgetItem(svId); ui->tableSAT->setItem(i,1,item);
                item = new QTableWidgetItem(cno); ui->tableSAT->setItem(i,2,item);
                item = new QTableWidgetItem(elevStr); ui->tableSAT->setItem(i,3,item);
                item = new QTableWidgetItem(azimStr); ui->tableSAT->setItem(i,4,item);
                item = new QTableWidgetItem(prRes); ui->tableSAT->setItem(i,5,item);
                item = new QTableWidgetItem(qualityIndStr); ui->tableSAT->setItem(i,6,item);
                item = new QTableWidgetItem(svUsed); ui->tableSAT->setItem(i,7,item);
                item = new QTableWidgetItem(healthStr); ui->tableSAT->setItem(i,8,item);
                item = new QTableWidgetItem(diffCor); ui->tableSAT->setItem(i,9,item);
                item = new QTableWidgetItem(smothed); ui->tableSAT->setItem(i,10,item);
                item = new QTableWidgetItem(orbitSourceStr); ui->tableSAT->setItem(i,11,item);
                item = new QTableWidgetItem(eph); ui->tableSAT->setItem(i,12,item);
                item = new QTableWidgetItem(alm); ui->tableSAT->setItem(i,13,item);
                item = new QTableWidgetItem(ano); ui->tableSAT->setItem(i,14,item);
                item = new QTableWidgetItem(aop); ui->tableSAT->setItem(i,15,item);
                i++;
            }
            setupTableSize(ui->tableSAT);

        }
    }
    if (protocol == "Unicore"){
        UnicoreParser parser(currentConnection);
        UnicoreMessage mess = parser.parseMessage(&streamBuffer);
        if (mess.data.isEmpty()) return;
        if (!mess.isAscii) return;
        if (mess.isCommand && mess.data.contains("command") && mess.data.contains("OK")){
            qDebug() << "OK";
        }
        else if (mess.isCommand && mess.data.contains("command") && !mess.data.contains("OK")){
            qDebug() << "NOT OK: "  << mess.data;
        }
        if (mess.asciiHeader.messageName == "BESTNAVA"){
            QList<QByteArray> fields = mess.data.split(',');
            QList<QString> fieldsStr;
            foreach (QByteArray field, fields) {
                fieldsStr.append(QString::fromLatin1(field));
            }
            QList<QLabel*> labels = {ui->labelBESTNAVPsol, ui->labelBESTNAVPostype, ui->labelBESTNAVLat,
                                       ui->labelBESTNAVLon, ui->labelBESTNAVHgt, ui->labelBESTNAVUnd,
                                       ui->labelBESTNAVdatum, ui->labelBESTNAVlatsko, ui->labelBESTNAVlonsko,
                                       ui->labelBESTNAVhgtsko, ui->labelBESTNAVbaseID, ui->labelBESTNAVdiffageP,
                                       ui->labelBESTNAVsolage, ui->labelBESTNAVsvs, ui->labelBESTNAVsvsused,
                                       nullptr,nullptr,nullptr,ui->labelBESTNAVextsolstat, ui->labelBESTNAVgalmask,
                                       ui->labelBESTNAVgpsmask, ui->labelBESTNAVVsol, ui->labelBESTNAVveltype,
                                       ui->labelBESTNAVlatency, ui->labelBESTNAVdiffageV, ui->labelBESTNAVhspeed,
                                      ui->labelBESTNAVtrkgnd, ui->labelBESTNAVvspeed, ui->labelBESTNAVvspeedsko, ui->labelBESTNAVhspeedsko};
            int index = -1;
            foreach (auto field, fieldsStr) {
                index++;
                if (index >= labels.count()) break;
                if (labels.at(index) == nullptr) continue;
                labels.at(index)->setText(field);
            }
            ui->lineRecieverDatumId->setText(ui->labelBESTNAVdatum->text());
        }
        else if (mess.asciiHeader.messageName == "GPSEPHA"){
            qDebug() << mess.data;
            QList<QByteArray> fields = mess.data.split(',');
            QList<QString> fieldsStr;
            foreach (QByteArray field, fields) {
                fieldsStr.append(QString::fromLatin1(field));
            }
            QList<QLabel*> labels;
            QGridLayout* layout = qobject_cast<QGridLayout*>(ui->frameGPSEPH->layout());
            for(int i = 0; i < layout->rowCount(); i++){
                labels.append(qobject_cast<QLabel*>(layout->itemAtPosition(i,1)->widget()));
            }
            int index = -1;
            foreach (auto field, fieldsStr) {
                index++;
                if (index >= labels.count()) break;
                if (labels.at(index) == nullptr) continue;
                labels.at(index)->setText(field);
            }
        }
        else if (mess.asciiHeader.messageName == "OBSVMA"){
            qDebug() << mess.data;
            ui->tableOBSVM->clearContents();
            ui->tableOBSVM->setRowCount(0);
            QList<QByteArray> fields = mess.data.split(',');
            QList<QString> fieldsStr;
            foreach (QByteArray field, fields) {
                fieldsStr.append(QString::fromLatin1(field));
            }
            int index = 0;
            QString num = fieldsStr.at(index);
            ui->labelOBSVMnum->setText(num);
            for(int i = 0; i < num.toUInt(); i++){
                int row = i;
                ui->tableOBSVM->insertRow(row);
                for(int j = 0; j<11; j++){
                    index++;
                    if (j == 8) continue;
                    int column = j;
                    QTableWidgetItem *item = new QTableWidgetItem(fieldsStr.at(index));
                    ui->tableOBSVM->setItem(row, column, item);
                }
            }
            setupTableSize(ui->tableOBSVM);
        }
        else if (mess.asciiHeader.messageName == "UNIHEADINGA"){
            qDebug() << mess.data;
            QList<QByteArray> fields = mess.data.split(',');
            QList<QString> fieldsStr;
            foreach (QByteArray field, fields) {
                fieldsStr.append(QString::fromLatin1(field));
            }
            QList<QLabel*> labels = {ui->labelUNIHEADINGsolstat, ui->labelUNIHEADINGpostype, ui->labelUNIHEADINGlen,
                                     ui->labelUNIHEADINGheading, ui->labelUNIHEADINGpitch, nullptr, ui->labelUNIHEADINGheadingsko,
                                       ui->labelUNIHEADINGpitchsko, ui->labelUNIHEADINGbaseID, ui->labelUNIHEADINGtracked, ui->labelUNIHEADINGused,
                                       ui->labelUNIHEADINGabove, ui->labelUNIHEADINGl2above, nullptr, ui->labelUNIHEADINGextsolstat,
                                      ui->labelUNIHEADINGgalmask, ui->labelUNIHEADINGgpsmask};
            int index = -1;
            foreach (auto field, fieldsStr) {
                index++;
                if (index >= labels.count()) break;
                if (labels.at(index) == nullptr) continue;
                labels.at(index)->setText(field);
            }
        }
        else if (mess.isCommand && mess.data.split(',').at(1) == "MASK"){
            QList<QByteArray> fields = mess.data.split(',').at(2).split(' ');
            QList<QString> data;
            foreach (QByteArray field, fields) {
                data.append(QString::fromLatin1(field));
            }
            // qDebug() << data;
            QMap<QString, QCheckBox*> checkBoxMap;
            checkBoxMap["GPSL1C"] = ui->checkMASKGPSl1ca;
            checkBoxMap["GPSL2C"] = ui->checkMASKGPSl2c;
            checkBoxMap["BDSB1I"] = ui->checkMASKBDSb1;
            checkBoxMap["BDSB2I"] = ui->checkMASKBDSb2;
            checkBoxMap["GLOL1"] = ui->checkMASKGLOr1;
            checkBoxMap["GLOL2"] = ui->checkMASKGLOr2;
            checkBoxMap["GALE1"] = ui->checkMASKGALe1;
            checkBoxMap["GALE5b"] = ui->checkMASKGALe5b;
            checkBoxMap["QZSSL1CA"] = ui->checkMASKQZSSq1ca;
            checkBoxMap["QZSSL2C"] = ui->checkMASKQZSSq2c;
            checkBoxMap["GPS"] = ui->checkMASKGPS;
            checkBoxMap["GLO"] = ui->checkMASKGLO;
            checkBoxMap["BDS"] = ui->checkMASKBDS;
            checkBoxMap["GAL"] = ui->checkMASKGAL;
            checkBoxMap["QZSS"] = ui->checkMASKQZSS;
            QMap<QString, QSpinBox*> spinBoxMap;
            spinBoxMap["GPS"] = ui->spinMASKGPS;
            spinBoxMap["BDS"] = ui->spinMASKBDS;
            spinBoxMap["GLO"] = ui->spinMASKGLO;
            spinBoxMap["QZSS"] = ui->spinMASKQZSS;
            spinBoxMap["GAL"] = ui->spinMASKGAL;
            QMap<QString, QList<int>> systemIndexMap;
            systemIndexMap["GPSL1C"] = {0};
            systemIndexMap["GPSL2C"] = {1};
            systemIndexMap["BDSB1I"] = {5};
            systemIndexMap["BDSB2I"] = {6};
            systemIndexMap["GLOL1"] = {11};
            systemIndexMap["GLOL2"] = {12};
            systemIndexMap["GALE1"] = {3};
            systemIndexMap["GALE5b"] = {4};
            systemIndexMap["QZSSL1CA"] = {8};
            systemIndexMap["QZSSL2C"] = {10};
            systemIndexMap["GPS"] = {0,1};
            systemIndexMap["GLO"] = {11,12};
            systemIndexMap["BDS"] = {5,6};
            systemIndexMap["GAL"] = {3,4};
            systemIndexMap["QZSS"] = {8,10};
            if (checkBoxMap[data.at(1)]){
                checkBoxMap[data.at(1)]->setChecked(true);
                foreach (int index, systemIndexMap[data.at(1)]) {
                    ui->comboRecieverSystems->setitemCheckState(index, true);
                }
            }
            if (data.count() > 2){
                if (spinBoxMap[data.at(2)]){
                    spinBoxMap[data.at(2)]->setValue(data.at(1).toFloat());
                    QList<int> minElevs = {ui->spinMASKGPS->value(), ui->spinMASKBDS->value(),
                                           ui->spinMASKGLO->value(), ui->spinMASKQZSS->value(), ui->spinMASKGAL->value()};
                    ui->spinRecieverMinElev->setValue(*std::min_element(minElevs.begin(), minElevs.end()));
                }
            }
            if (data.at(1) == "CN0"){
                ui->spinMASKCN0->setValue(data.at(2).toFloat());
                ui->spinRecieverCNOThresh->setValue(data.at(2).toFloat());
            }
        }
        else if (mess.asciiHeader.messageName == "MODE"){
            qDebug() << mess.data;
            QList<QByteArray> fields = mess.data.split(',').at(0).split(' ');
            qDebug() << fields;
            QList<QString> fieldsStr;
            foreach (QByteArray field, fields) {
                fieldsStr.append(QString::fromLatin1(field));
            }
            QString currMode;
            for (int wordIndex = 1; wordIndex < fieldsStr.count(); ++wordIndex) {
                QString word = fieldsStr.at(wordIndex);
                currMode += word;
                if (wordIndex != fieldsStr.count() - 1) currMode+=' ';
            }
            qDebug() << currMode;
            ui->comboMODE->setCurrentText(currMode);
            QMap<QString,QString> modeMap;
            modeMap["ROVER"] = "Portable";
            modeMap["BASE"] = "Stationary";
            modeMap["ROVER SURVEY"] = "Pedestrian";
            modeMap["ROVER AUTOMOTIVE"] = "Automotive";
            modeMap["ROVER UAV"] = "uav";
            modeMap[""] = "Other";
            ui->comboRecieverMode->setCurrentText(modeMap[currMode]);
        }
        else if (mess.isCommand && mess.data.split(',').at(0) == "CONFIG" && mess.data.split(',').at(1) == "RTK"){
            QList<QByteArray> fields = mess.data.split(',').at(2).split(' ');
            QList<QString> data;
            foreach (QByteArray field, fields) {
                data.append(QString::fromLatin1(field));
            }
            if (data.at(2) == "USER_DEFAULTS" || data.at(2) == "DISABLE" || data.at(2) == "RESET"){
                ui->comboCONFIGRTK->setCurrentText(data.at(2));
                bool check = data.at(2) == "USER_DEFAULTS" || data.at(2) == "RESET";
                ui->checkRecieverRTKSolution->setChecked(check);
            }
            if (data.at(2) == "TIMEOUT"){
                ui->spinCONFIGRTK->setValue(data.at(3).toInt());
            }
            if (data.at(2) == "RELIABILITY"){
                ui->comboCONFIGRTKrel->setCurrentIndex(data.at(3).toInt() - 1);
            }
        }
        else if (mess.isCommand && mess.data.split(',').at(0) == "CONFIG" && mess.data.split(',').at(1) == "DGPS"){
            QList<QByteArray> fields = mess.data.split(',').at(2).split(' ');
            QList<QString> data;
            foreach (QByteArray field, fields) {
                data.append(QString::fromLatin1(field));
            }
            if (data.at(2) == "TIMEOUT"){
                ui->spinDGPS->setValue(data.at(3).toInt());
            }
        }
        else if (mess.isCommand && mess.data.split(',').at(0) == "CONFIG" && mess.data.split(',').at(1) == "ANTIJAM"){
            QList<QByteArray> fields = mess.data.split(',').at(2).split(' ');
            QList<QString> data;
            foreach (QByteArray field, fields) {
                data.append(QString::fromLatin1(field));
            }
            ui->comboCONFIGANTIJAM->setCurrentText(data.at(2));
            bool check = data.at(2) == "AUTO" || data.at(2) == "FORCE";
            ui->checkRecieverJamming->setChecked(check);
        }
        else if (mess.asciiHeader.messageName == "UNILOGLIST"){
            QList<QByteArray> fields = mess.data.split('<');
            QList<QString> fieldsStr;
            foreach (QByteArray field, fields) {
                fieldsStr.append(QString::fromLatin1(field).remove('\t').remove('\r').remove('\n'));
            }
            fieldsStr.removeFirst();
            fieldsStr.removeFirst();
            bool bestnavCheck = false;
            bool obsvmCheck = false;
            bool uniheadingCheck = false;
            bool gpsephCheck = false;
            bool bd3ephCheck = false;
            bool gloephCheck = false;
            bool galephCheck = false;
            foreach (QString info, fieldsStr) {
                if (info.split(' ').at(0) == "BESTNAVA"){
                    bestnavCheck = true;
                    ui->spinRecieverPos3->setValue(info.split(' ').at(2).toInt());
                    ui->spinRecieverVel3->setValue(info.split(' ').at(2).toInt());
                    ui->checkRecieverPosLLH->setChecked(true);
                    ui->checkRecieverVelNED->setChecked(true);
                }
                if (info.split(' ').at(0) == "BESTNAVXYZA"){
                    bestnavCheck = true;
                    ui->spinRecieverPos3->setValue(info.split(' ').at(2).toInt());
                    ui->spinRecieverVel3->setValue(info.split(' ').at(2).toInt());
                    ui->checkRecieverPosECEF->setChecked(true);
                    ui->checkRecieverVelECEF->setChecked(true);
                }
                if (info.split(' ').at(0) == "OBSVMA"){
                    obsvmCheck = true;
                    ui->spinRecieverRaw3->setValue(info.split(' ').at(2).toInt());
                }
                if (info.split(' ').at(0) == "UNIHEADINGA"){
                    uniheadingCheck = true;
                    ui->spinRecieverRelPos3->setValue(info.split(' ').at(2).toInt());
                }
                if (info.split(' ').at(0) == "GPSEPHA"){
                    gpsephCheck = true;
                    ui->spinRecieverEph3->setValue(info.split(' ').at(2).toInt());
                }
                if (info.split(' ').at(0) == "BD3EPHA"){
                    bd3ephCheck = true;
                    ui->spinRecieverEph3->setValue(info.split(' ').at(2).toInt());
                }
                if (info.split(' ').at(0) == "GLOEPHA"){
                    gloephCheck = true;
                    ui->spinRecieverEph3->setValue(info.split(' ').at(2).toInt());
                }
                if (info.split(' ').at(0) == "GALEPHA"){
                    galephCheck = true;
                    ui->spinRecieverEph3->setValue(info.split(' ').at(2).toInt());
                }
            }
            bool ephCheck = gpsephCheck & bd3ephCheck & gloephCheck & galephCheck;
            ui->checkRecieverPos->setChecked(bestnavCheck);
            ui->checkRecieverVel->setChecked(bestnavCheck);
            ui->checkRecieverRaw->setChecked(obsvmCheck);
            ui->checkRecieverRelpos->setChecked(uniheadingCheck);
            ui->checkRecieverEph->setChecked(ephCheck);
            if (!ephCheck) ui->spinRecieverEph3->setValue(0);
        }
    }
}


void deviceConfigurationsDialog::pollConfigMessages(QString msgType){
    sendPoll();
}

void deviceConfigurationsDialog::sendMSGPoll(U1 classID, U1 messageID)
{
    U1 hdrdata[] = {0xb5, 0x62};
    QByteArray hdr(reinterpret_cast<char*>(hdrdata), sizeof(hdrdata));
    U1 endata[] = {CFG::MSG::classID, CFG::MSG::messageID, 0x02, 0x00};
    QByteArray en(reinterpret_cast<char*>(endata), sizeof(endata));
    U1 payloaddata[] = {classID, messageID};
    QByteArray payload(reinterpret_cast<char*>(payloaddata), sizeof(payloaddata));
    QByteArray msg = hdr + en + payload;
    QByteArray checkSum = UbloxParser::calcCheckSum(en + payload);
    msg.append(checkSum);
    UbloxParser parser(currentConnection);
    parser.sendMessage(msg);
}

/// Он будет вызываться при выборе сообщения и нажатии на кнопку запроса
void deviceConfigurationsDialog::sendPoll()
{
    if (protocol == "Ublox"){
        QByteArray msg;
        if (currentItemText == "CFG-MSG"){
            QComboBox* comboMSG = ui->comboMSG;
            if(comboMSG->currentText().isEmpty()){
                return;
            }
            U1 classID = messagesIDMapUBX[comboMSG->currentText()].first;
            U1 messageID = messagesIDMapUBX[comboMSG->currentText()].second;
            sendMSGPoll(classID, messageID);
            return;
        }
        else if (currentItemText == "CFG-PRT"){
            U1 hdrdata[] = {0xb5, 0x62};
            QByteArray hdr(reinterpret_cast<char*>(hdrdata), sizeof(hdrdata));
            U1 endata[] = {CFG::PRT::classID, CFG::PRT::messageID, 0x01, 0x00};
            QByteArray en(reinterpret_cast<char*>(endata), sizeof(endata));
            U1 payloadata = ui->comboPRTTarget->currentText().section(' ',0,0).toUInt();
            QByteArray payload;payload.resize(1); payload[0] = payloadata;
            msg = hdr + en + payload;
            QByteArray checkSum = UbloxParser::calcCheckSum(en + payload);
            msg.append(checkSum);
        }
        else if (currentItemText.left(3) == "NAV"){
            QString text = currentItemText;
            QString messName = text.remove("NAV-");
            qDebug() << messName;
            msg = UbloxParser::createPollMessage(messagesIDMapUBX[messName].first, messagesIDMapUBX[messName].second);
        }
        else if (currentItemText.left(3) == "CFG"){
            QString text = currentItemText;
            QString messName = text.remove("CFG-");
            qDebug() << messName;
            msg = UbloxParser::createPollMessage(messagesIDMapUBX[messName].first, messagesIDMapUBX[messName].second);
        }
        else return;

        UbloxParser parser(currentConnection);
        parser.sendMessage(msg);
    }
    else if (protocol == "Unicore"){
        UnicoreParser parser(currentConnection);
        QList<QString> messages = {"BESTNAV", "GPSEPH", "OBSVM", "UNIHEADING"};
        if (messages.contains(currentItemText)){
            parser.sendMessage(currentItemText + 'A');
        }
        else if (currentItemText.contains("CONFIG")){
            parser.sendMessage("CONFIG");
        }
        else{
            parser.sendMessage(currentItemText);
        }
    }
}

void deviceConfigurationsDialog::changeListsSelection(){
    QRadioButton* radio = qobject_cast<QRadioButton*>(sender());
    QString text = radio->text();
    QList<QListWidget*> lists = {ui->listCFGClear, ui->listCFGSave, ui->listCFGLoad};
    foreach (auto list, lists) {
        list->clearSelection();
        list->setEnabled(false);
    }
    if (text == "Revert to last saved configuration"){
        for (int i = 0; i<16; i++){
            ui->listCFGLoad->item(i)->setSelected(true);
        }
    }
    else if (text == "Revert all but ANT default configuration"){
        for (int i = 0; i<16; i++){
            ui->listCFGLoad->item(i)->setSelected(true);
            ui->listCFGClear->item(i)->setSelected(true);
        }
        ui->listCFGClear->item(10)->setSelected(false);
    }
    else if (text == "Revert to default configuration"){
        for (int i = 0; i<16; i++){
            ui->listCFGLoad->item(i)->setSelected(true);
            ui->listCFGClear->item(i)->setSelected(true);
        }
    }
    else if (text == "Save current configuration"){
        for (int i = 0; i<16; i++){
            ui->listCFGSave->item(i)->setSelected(true);
        }
    }
    else if (text == "User Defined operation"){
        foreach (auto list, lists) {
            list->clearSelection();
            list->setEnabled(true);
        }
    }
}

void deviceConfigurationsDialog::changeBBRSelection(QString text)
{
    QListWidget* list = ui->listBBR;
    ui->listBBR->clearSelection();
    if (text == "Hot start"){
        ui->listBBR->setEnabled(false);
        list->clearSelection();
    }
    else if (text == "Warm start"){
        ui->listBBR->setEnabled(false);
        list->item(0)->setSelected(true);
    }
    else if (text == "Cold start"){
        ui->listBBR->setEnabled(false);
        list->selectAll();
    }
    else if (text == "User Defined"){
        list->setEnabled(true);
    }
}

void deviceConfigurationsDialog::sendSet()
{
    qDebug() << currentItemText;
    if (protocol == "Ublox"){
        if (currentItemText == "CFG-MSG"){
            QComboBox* comboMSG = ui->comboMSG;
            if(comboMSG->currentText().isEmpty()){
                return;
            }
            QSpinBox* spinI2C =   ui->spinMSGI2C;
            QSpinBox* spinUART1 = ui->spinMSGUART1;
            QSpinBox* spinUART2 = ui->spinMSGUART2;
            QSpinBox* spinUSB =   ui->spinMSGUSB;
            QSpinBox* spinSPI =   ui->spinMSGSPI;
            QByteArray hdr;
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
            payload[0] = messagesIDMapUBX[comboMSG->currentText()].first;
            payload[1] = messagesIDMapUBX[comboMSG->currentText()].second;
            payload[2] = (uint8_t)spinI2C->value();
            payload[3] = (uint8_t)spinUART1->value();
            payload[4] = (uint8_t)spinUART2->value();
            payload[5] = (uint8_t)spinUSB->value();
            payload[6] = (uint8_t)spinSPI->value();
            payload[7] = 0x00; //без понятия при каких обстоятельствах он может превратиться во что-то кроме 0x00
            QByteArray msg = hdr + en + payload;
            QByteArray checkSum = UbloxParser::calcCheckSum(en + payload);
            msg.append(checkSum);
            UbloxParser parser(currentConnection);
            parser.sendMessage(msg);
        }
        else if(currentItemText == "CFG-CFG"){
            QListWidget* listMem = ui->listCFGMem;
            U1 hdrdata[] = {0xb5, 0x62};
            QByteArray hdr(reinterpret_cast<char*>(hdrdata), sizeof(hdrdata));
            U1 endata[] = {CFG::_CFG::classID, CFG::_CFG::messageID, 0x0d, 0x00};
            U1 memByte = 1 << listMem->currentItem()->text().left(1).toUInt();
            QByteArray en(reinterpret_cast<char*>(endata), sizeof(endata));
            // 30 ->   x + 1 << 30
            QByteArray payload;
            QDataStream stream(&payload, QIODevice::WriteOnly);
            stream.setByteOrder(QDataStream::LittleEndian);

            QList<QListWidget*> lists = {ui->listCFGClear, ui->listCFGSave, ui->listCFGLoad};
            foreach (auto list, lists) {
                U4 value = 0x00;
                for (int i = 0; i<list->count(); i++){
                    value+= list->item(i)->isSelected() << i;
                }
                stream << value;
            }
            payload.append(memByte);
            qDebug() << payload.toHex(' ');
            QByteArray msg = hdr + en + payload;
            QByteArray checkSum = UbloxParser::calcCheckSum(en + payload);
            msg.append(checkSum);
            UbloxParser parser(currentConnection);
            parser.sendMessage(msg);
        }
        else if(currentItemText == "CFG-DGNSS"){
            QComboBox* comboDGNSS = ui->comboDGNSS;
            U1 dgnssMode = comboDGNSS->currentText().left(1).toUInt();
            U1 hdrdata[] = {0xb5, 0x62};
            QByteArray hdr(reinterpret_cast<char*>(hdrdata), sizeof(hdrdata));
            U1 endata[] = {CFG::DGNSS::classID, CFG::DGNSS::messageID, 0x04, 0x00};
            QByteArray en(reinterpret_cast<char*>(endata), sizeof(endata));
            U1 payloadData[] = {dgnssMode, 0x00, 0x00, 0x00};
            QByteArray payload(reinterpret_cast<char*>(payloadData), sizeof(payloadData));
            qDebug() << payload.toHex(' ');
            QByteArray msg = hdr + en + payload;
            QByteArray checkSum = UbloxParser::calcCheckSum(en + payload);
            msg.append(checkSum);
            UbloxParser parser(currentConnection);
            parser.sendMessage(msg);
        }
        else if(currentItemText == "CFG-RST"){
            // QComboBox* comboBBR = ui->comboRSTBBR;
            QComboBox* comboMode = ui->comboRSTMode;
            QListWidget* listBBR = ui->listBBR;
            U1 hdrdata[] = {0xb5, 0x62};
            QByteArray hdr(reinterpret_cast<char*>(hdrdata), sizeof(hdrdata));
            U1 endata[] = {CFG::RST::classID, CFG::RST::messageID, 0x04, 0x00};
            QByteArray en(reinterpret_cast<char*>(endata), sizeof(endata));
            U2 navBbrMask = 0x00;
            for (int i = 0; i<listBBR->count(); i++){
                if (i == listBBR->count()-1){
                    navBbrMask+= listBBR->item(i)->isSelected() << 15;
                }
                else navBbrMask+= listBBR->item(i)->isSelected() << i;
            }
            U1 resetMode = comboMode->currentText().left(1).toUInt();
            U1 reserved = 0x00;
            QByteArray payload;
            QDataStream stream(&payload, QIODevice::WriteOnly);
            stream.setByteOrder(QDataStream::LittleEndian);
            stream << navBbrMask << resetMode << reserved;
            qDebug() << payload.toHex(' ');
            QByteArray msg = hdr + en + payload;
            QByteArray checkSum = UbloxParser::calcCheckSum(en + payload);
            msg.append(checkSum);
            UbloxParser parser(currentConnection);
            parser.sendMessage(msg);
        }
        else if(currentItemText == "CFG-RATE"){
            QSpinBox* spinPeriod = ui->spinRATEPer;
            QSpinBox* spinRate = ui->spinRATE;
            QComboBox*  comboSource = ui->comboRATE;
            U1 hdrdata[] = {0xb5, 0x62};
            QByteArray hdr(reinterpret_cast<char*>(hdrdata), sizeof(hdrdata));
            U1 endata[] = {CFG::RATE::classID, CFG::RATE::messageID, 0x06, 0x00};
            QByteArray en(reinterpret_cast<char*>(endata), sizeof(endata));
            U2 measRate = spinPeriod->value();
            U2 navRate = spinRate->value();
            U2 timeRef = comboSource->currentIndex();
            QByteArray payload;
            QDataStream stream(&payload, QIODevice::WriteOnly);
            stream.setByteOrder(QDataStream::LittleEndian);
            stream << measRate << navRate << timeRef;
            qDebug() << payload.toHex(' ');
            QByteArray msg = hdr + en + payload;
            QByteArray checkSum = UbloxParser::calcCheckSum(en + payload);
            msg.append(checkSum);
            UbloxParser parser(currentConnection);
            parser.sendMessage(msg);
        }
        else if(currentItemText == "CFG-PRT"){
            U1 hdrdata[] = {0xb5, 0x62};
            QByteArray hdr(reinterpret_cast<char*>(hdrdata), sizeof(hdrdata));
            U1 endata[] = {CFG::PRT::classID, CFG::PRT::messageID, 0x14, 0x00};
            QByteArray en(reinterpret_cast<char*>(endata), sizeof(endata));
            QByteArray payload;
            QDataStream stream(&payload, QIODevice::WriteOnly);
            stream.setByteOrder(QDataStream::LittleEndian);
            U1 portId = ui->comboPRTTarget->currentText().section(' ', 0, 0).toUInt();
            if (portId == 3){
                U1 reserved1 = 0x00;
                X2 txReady;
                U4 reserved2 = 0x00;
                U4 reserved3 = 0x00;
                X2 inProtoMask = ui->comboPRTIn->currentText().section(' ', 0, 0).toUInt();
                X2 outProtoMask = ui->comboPRTOut->currentText().section(' ', 0, 0).toUInt();
                U4 reserved4 = ui->checkTXTimeout->isChecked() << 17;
                bool en = ui->checkTXEn->isChecked();
                bool pol = ui->checkTXPol->isChecked();
                U1 pin = ui->spinTXPin->value();
                U2 threshhold = ui->spinTXThreshhold->value()/8;
                txReady = en + (pol << 1) + (pin << 2) + (threshhold << 7);
                stream << portId << reserved1 << txReady << reserved2 << reserved3 << inProtoMask  << outProtoMask << reserved4;
            }
            else if (portId == 1 || portId == 2){
                U1 reserved1 = 0x00;
                X2 txReady;
                X4 mode = (ui->comboUARTDataBits->currentIndex() << 6) + (ui->comboUARTParity->currentText().section(' ',0,0).toUInt() << 9) + (ui->comboUARTStopBits->currentIndex() << 12);
                U4 baudRate = ui->comboUARTBaud->currentText().toUInt();
                X2 inProtoMask = ui->comboPRTIn->currentText().section(' ', 0, 0).toUInt();
                X2 outProtoMask = ui->comboPRTOut->currentText().section(' ', 0, 0).toUInt();
                X2 flags = ui->checkTXTimeout->isChecked() << 1;
                U2 reserved2 = 0x00;
                bool en = ui->checkTXEn->isChecked();
                bool pol = ui->checkTXPol->isChecked();
                U1 pin = ui->spinTXPin->value();
                U2 threshhold = ui->spinTXThreshhold->value()/8;
                txReady = en + (pol << 1) + (pin << 2) + (threshhold << 7);
                stream <<  portId << reserved1 << txReady << mode << baudRate << inProtoMask << outProtoMask << flags << reserved2;
            }
            qDebug() << payload.toHex(' ');
            QByteArray msg = hdr + en + payload;
            QByteArray checkSum = UbloxParser::calcCheckSum(en + payload);
            msg.append(checkSum);
            UbloxParser parser(currentConnection);
            parser.sendMessage(msg);
        }
        else if(currentItemText == "CFG-NAV5"){

            U1 hdrdata[] = {0xb5, 0x62};
            QByteArray hdr(reinterpret_cast<char*>(hdrdata), sizeof(hdrdata));
            U1 endata[] = {CFG::NAV5::classID, CFG::NAV5::messageID, 0x24, 0x00};
            QByteArray en(reinterpret_cast<char*>(endata), sizeof(endata));
            U2 mask = 0xffff;
            U1 dynMode1 = ui->comboNAV5DynModel->currentText().section(' ',0,0).toUInt();
            U1 fixMode = ui->comboNAV5FixMode->currentText().section(' ',0,0).toUInt();
            I4 fixedAlt = (I4)(ui->spinNAV5FixAlt->value()*100);
            U4 fixedAltVar = (U4)(ui->spinNAV5AltVar->value()*10000);
            I1 minElev = ui->spinNAV5MinElev->value();
            U1 drLimit = ui->spinNAV5DRLimit->value();
            U2 pDop = (U2)(ui->spinNAV5PDOP->value()*10);
            U2 tDop = (U2)(ui->spinNAV5TDOP->value()*10);
            U2 pAcc = ui->spinNAV5PAcc->value();
            U2 tAcc = ui->spinNAV5TAcc->value();
            U1 staticHoldThresh = (U1)(ui->spinNAV5SHT->value()*100);
            qDebug() << fixedAlt << fixedAltVar << pDop << tDop << staticHoldThresh;
            U1 dgnssTimeout = ui->spinNAV5DGNSSTimeout->value();
            U1 cnoThreshNumSVs = ui->spinNAV5CNOThresh->value();
            U1 cnoThresh = ui->spinNAV5CNOThresh->value();
            U1 reserved1 = 0x00;
            U1 reserved2 = 0x00;
            U2 staticHoldMaxDist = ui->spinNAV5MaxDist->value();
            U1 utcStandard = ui->comboNAV5UTC->currentText().section(' ',0,0).toUInt();;
            U1 reserved3 = 0x00;
            U1 reserved4 = 0x00;
            U1 reserved5 = 0x00;
            U1 reserved6 = 0x00;
            U1 reserved7 = 0x00;
            QByteArray payload;
            QDataStream stream(&payload, QIODevice::WriteOnly);
            stream.setByteOrder(QDataStream::LittleEndian);

            stream << mask << dynMode1 << fixMode << fixedAlt << fixedAltVar << minElev << drLimit << pDop << tDop << pAcc << tAcc
                   << staticHoldThresh << dgnssTimeout << cnoThreshNumSVs << cnoThresh << reserved1 << reserved2 << staticHoldMaxDist
                   << utcStandard << reserved3 << reserved4 << reserved5 << reserved6 << reserved7;

            qDebug() << payload.toHex(' ');
            QByteArray msg = hdr + en + payload;
            QByteArray checkSum = UbloxParser::calcCheckSum(en + payload);
            msg.append(checkSum);
            UbloxParser parser(currentConnection);
            parser.sendMessage(msg);
        }
        else if(currentItemText == "CFG-GNSS"){
            U1 hdrdata[] = {0xb5, 0x62};
            QByteArray hdr(reinterpret_cast<char*>(hdrdata), sizeof(hdrdata));
            U1 endata[] = {CFG::GNSS::classID, CFG::GNSS::messageID, 0x3C, 0x00};
            QByteArray en(reinterpret_cast<char*>(endata), sizeof(endata));
            QByteArray payload;
            QDataStream stream(&payload, QIODevice::WriteOnly);
            stream.setByteOrder(QDataStream::LittleEndian);
            U1 msgVer = 0x00;
            U1 numTrkChHw = 0x00;
            U1 numTrkChUse = ui->spinGNSSUseCh->value();
            U1 numConfigBlocks = 0x07;
            stream << msgVer << numTrkChHw << numTrkChUse << numConfigBlocks;
            for (int i = 0; i<numConfigBlocks; i++){
                U1 gnssId = i;
                QGridLayout* layout = qobject_cast<QGridLayout*>(ui->frameGNSS->layout());
                int row = gnssId + 1;
                int column = 2;
                U1 resTrkCh = qobject_cast<QSpinBox*>(layout->itemAtPosition(row,column+2)->widget())->value();
                U1 maxTrkCh = qobject_cast<QSpinBox*>(layout->itemAtPosition(row,column+3)->widget())->value();
                U1 reserved = 0x00;
                bool enabled = qobject_cast<QCheckBox*>(layout->itemAtPosition(row,column+1)->widget())->isChecked();
                U4 sigCfgMask;
                bool L1C;
                bool L2C;
                bool E1;
                bool E5;
                bool B1;
                bool B2;
                bool L1;
                bool L1S;
                bool L2;
                switch (i) {
                case 0x00:
                    L1C = ui->checkGNSSGPSSigL1C->isChecked();
                    L2C = ui->checkGNSSGPSSigL2C->isChecked();
                    sigCfgMask = L1C + (L2C << 4);
                    break;
                case 0x01:
                    L1C = ui->checkGNSSSBASSigL1C->isChecked();
                    sigCfgMask = L1C;
                    break;
                case 0x02:
                    E1 = ui->checkGNSSGalSigE1->isChecked();
                    E5 = ui->checkGNSSGalSigE5->isChecked();
                    sigCfgMask = E1 + (E5 << 5);
                    break;
                case 0x03:
                    B1 = ui->checkGNSSBeiSigB1->isChecked();
                    B2 = ui->checkGNSSBeiSigB2->isChecked();
                    sigCfgMask = B1 + (B2 << 4);

                    break;
                case 0x04:
                    L1 = ui->checkGNSSIMESSigL1C->isChecked();
                    sigCfgMask = L1;
                    break;
                case 0x05:
                    L1C = ui->checkGNSSQZSSSigL1C->isChecked();
                    L1S = ui->checkGNSSQZSSSigL1S->isChecked();
                    L2C = ui->checkGNSSQZSSSigL2C->isChecked();
                    sigCfgMask = L1C + (L1S << 2) + (L2C << 4);
                    break;
                case 0x06:
                    L1 = ui->checkGNSSGLOSigL1->isChecked();
                    L2 = ui->checkGNSSGLOSigL2->isChecked();
                    sigCfgMask = L1 + (L2 << 4);
                    break;
                default:
                    break;
                }
                X4 flags = enabled + (sigCfgMask << 16);
                stream << gnssId << resTrkCh << maxTrkCh << reserved << flags;
            }

            qDebug() << payload.toHex(' ');
            QByteArray msg = hdr + en + payload;
            QByteArray checkSum = UbloxParser::calcCheckSum(en + payload);
            msg.append(checkSum);
            UbloxParser parser(currentConnection);
            parser.sendMessage(msg);
        }
        else if(currentItemText == "CFG-ITFM"){
            U1 hdrdata[] = {0xb5, 0x62};
            QByteArray hdr(reinterpret_cast<char*>(hdrdata), sizeof(hdrdata));
            U1 endata[] = {CFG::ITFM::classID, CFG::ITFM::messageID, 0x08, 0x00};
            QByteArray en(reinterpret_cast<char*>(endata), sizeof(endata));
            QByteArray payload;
            QDataStream stream(&payload, QIODevice::WriteOnly);
            stream.setByteOrder(QDataStream::LittleEndian);
            X4 config;
            X4 config2;
            U1 bbThreshold = ui->spinITFMbbThresh->value();
            U1 cwTheshhold = ui->spinITFMcwThresh->value();
            bool en1 = ui->checkITFMEn->checkState();
            U1 antType = ui->comboITFMantType->currentIndex();
            U4 algorithmBits =  0x16B156;
            U2 genetalBits = 0x31E;
            bool en2 = ui->comboITFMantType->currentIndex();
            config = bbThreshold + (cwTheshhold << 4) + (algorithmBits << 9) + (en1 << 31);
            config2 = genetalBits + (antType << 12) + (en2 << 14);
            stream << config << config2;
            qDebug() << payload.toHex(' ');
            QByteArray msg = hdr + en + payload;
            QByteArray checkSum = UbloxParser::calcCheckSum(en + payload);
            msg.append(checkSum);
            UbloxParser parser(currentConnection);
            parser.sendMessage(msg);
        }
    }
    else if (protocol == "Unicore"){
        UnicoreParser parser(currentConnection);
        if (currentItemText == "MASK"){
            QMap<QCheckBox*, QPair<QSpinBox*, QObjectList>> systemsMap;
            QPair<QSpinBox*, QObjectList> GPSpair;
            GPSpair.first = ui->spinMASKGPS;
            QObjectList gpsList = {ui->checkMASKGPSl1ca, ui->checkMASKGPSl2c};
            GPSpair.second = gpsList;
            QPair<QSpinBox*, QObjectList> BDSpair;
            BDSpair.first = ui->spinMASKBDS;
            QObjectList BDSList = {ui->checkMASKBDSb1, ui->checkMASKBDSb2};
            BDSpair.second = BDSList;
            QPair<QSpinBox*, QObjectList> GLOpair;
            GLOpair.first = ui->spinMASKGLO;
            QObjectList GLOList = {ui->checkMASKGLOr1, ui->checkMASKGLOr2};
            GLOpair.second = GLOList;
            QPair<QSpinBox*, QObjectList> GALpair;
            GALpair.first = ui->spinMASKGAL;
            QObjectList GALList = {ui->checkMASKGALe1, ui->checkMASKGALe5b};
            GALpair.second = GALList;
            QPair<QSpinBox*, QObjectList> QZSSpair;
            QZSSpair.first = ui->spinMASKQZSS;
            QObjectList QZSSList = {ui->checkMASKQZSSq1ca, ui->checkMASKQZSSq2c};
            QZSSpair.second = QZSSList;
            systemsMap[ui->checkMASKGPS] = GPSpair;
            systemsMap[ui->checkMASKBDS] = BDSpair;
            systemsMap[ui->checkMASKGLO] = GLOpair;
            systemsMap[ui->checkMASKGAL] = GALpair;
            systemsMap[ui->checkMASKQZSS] = QZSSpair;
            foreach (QCheckBox* key, systemsMap.keys()) {
                if (key->isChecked()){
                    parser.sendMessage("MASK " + key->text());
                }
                else{
                    parser.sendMessage("UNMASK " + key->text());
                    foreach (QObject* obj, systemsMap[key].second) {
                        QCheckBox* check = qobject_cast<QCheckBox*>(obj);
                        if (check->isChecked()){
                            parser.sendMessage("MASK " + check->text());
                        }
                        else{parser.sendMessage("UNMASK " + check->text());}
                    }
                }
                if (systemsMap[key].first->value()>=0){
                    parser.sendMessage("MASK " + QString::number(systemsMap[key].first->value()) + " " + key->text());
                }
            }
            parser.sendMessage("MASK CN0 " + QString::number(ui->spinMASKCN0->value()));
        }
        else if (currentItemText == "MODE"){
            parser.sendMessage("MODE " + ui->comboMODE->currentText());
        }
        else if (currentItemText == "CONFIG RTK"){
            parser.sendMessage("CONFIG RTK " + ui->comboCONFIGRTK->currentText());
            if (ui->frameCONFIGRTKparams->isEnabled()){
                parser.sendMessage("CONFIG RTK TIMEOUT " + QString::number(ui->spinCONFIGRTK->value()));
                parser.sendMessage("CONFIG RTK RELIABILITY " + QString::number(ui->comboCONFIGRTKrel->currentIndex()+1));
            }
        }
        else if (currentItemText == "CONFIG DGPS"){
            parser.sendMessage("CONFIG DGPS TIMEOUT " + QString::number(ui->spinDGPS->value()));
        }
        else if (currentItemText == "CONFIG ANTIJAM"){
            parser.sendMessage("CONFIG ANTIJAM " + ui->comboCONFIGANTIJAM->currentText());
        }
    }
    sendPoll();
}

void deviceConfigurationsDialog::saveConfig()
{

}

void deviceConfigurationsDialog::setChildrenHidden(QObject* parent, bool isHidden){
    foreach (QObject* object, parent->children()) {
        if(qobject_cast<QWidget*>(object)){
            QWidget *widget = qobject_cast<QWidget*>(object);
            widget->setHidden(isHidden);
        }
    }
}

void deviceConfigurationsDialog::setChildrenEnabled(QObject *parent, bool isEnabled){
    foreach (QObject* obj, parent->children()) {
        if (qobject_cast<QWidget*>(obj)){
            QWidget* widget = qobject_cast<QWidget*>(obj);
            widget->setEnabled(isEnabled);
        }
    }
}

void deviceConfigurationsDialog::updateMessageSettings(QTreeWidgetItem* item, int column)
{
    if (item->childCount()>0) return;
    QString text = item->text(column);
    this->currentItemText = text;
    if (currentItemText == "MASK"){
        QObjectList children = ui->frameMASK->children();
        foreach (QObject* obj, children) {
            if (qobject_cast<QWidget*>(obj)){
                QWidget* widget = qobject_cast<QWidget*>(obj);
                if (qobject_cast<QSpinBox*>(widget)){
                    qobject_cast<QSpinBox*>(widget)->setValue(0);
                }
                if (qobject_cast<QCheckBox*>(widget)){
                    qobject_cast<QCheckBox*>(widget)->setChecked(false);
                }
            }
        }
    }
    ui->labelMessageDescription->setText(messagesDescriptionsMap[text]);
    setChildrenHidden(ui->scrollAreaMessageSettingsContents, true);
    QFrame *currentFrame;
    currentFrame = framesMap[currentItemText];
    if (!currentFrame) return;
    setChildrenHidden(currentFrame,false);
    currentFrame->setHidden(false);
    sendPoll();
}

void deviceConfigurationsDialog::updateSettings()
{
    QComboBox* combo = ui->comboBoxDevice1;
    if (combo->currentIndex() == -1){
        return;
    }
    QPair<QString,QList<QString>> deviceInfo = devicesMap[combo->currentText()];
    QString protocol = deviceInfo.second.at(INDEX_GENERAL_PROTOCOL);
    if (protocol == "Ublox"){
        UbloxParser parser(currentConnection);
        parser.sendMessage(UbloxParser::createPollMessage(CFG::DAT::classID, CFG::DAT::messageID));
        parser.sendMessage(UbloxParser::createPollMessage(CFG::NAV5::classID, CFG::NAV5::messageID));
        parser.sendMessage(UbloxParser::createPollMessage(CFG::ITFM::classID, CFG::ITFM::messageID));
        parser.sendMessage(UbloxParser::createPollMessage(CFG::GNSS::classID, CFG::GNSS::messageID));
        parser.sendMessage(UbloxParser::createPollMessage(CFG::DGNSS::classID, CFG::DGNSS::messageID));
        sendMSGPoll(RXM::SFRBX::classID, RXM::SFRBX::messageID);
        sendMSGPoll(RXM::RAWX::classID, RXM::RAWX::messageID);
        sendMSGPoll(NAV::VELNED::classID, NAV::VELNED::messageID);
        sendMSGPoll(NAV::VELECEF::classID, NAV::VELECEF::messageID);
        sendMSGPoll(NAV::POSLLH::classID, NAV::POSLLH::messageID);
        sendMSGPoll(NAV::POSECEF::classID, NAV::POSECEF::messageID);
        sendMSGPoll(NAV::RELPOSNED::classID, NAV::RELPOSNED::messageID);
    }
    else if (protocol == "Unicore"){
        UnicoreParser parser(currentConnection);
        parser.sendMessage("MASK");
        parser.sendMessage("CONFIG");
        parser.sendMessage("UNILOGLIST");
        parser.sendMessage("MODE");
        parser.sendMessage("BESTNAVA");
    }
    ui->pushButtonSend->setEnabled(true);
}

void deviceConfigurationsDialog::sendSettings()
{
    QComboBox* combo = ui->comboBoxDevice1;
    if (combo->currentIndex() == -1){
        return;
    }
    if (protocol == "Ublox"){
        QByteArray msg;
        U1 hdrdata[] = {0xb5, 0x62};
        QByteArray hdr(reinterpret_cast<char*>(hdrdata), sizeof(hdrdata));
        U1 endata[4];
        QByteArray en;
        QByteArray checkSum;
        QByteArray payload;
        QDataStream stream(&payload, QIODevice::WriteOnly);
        stream.setByteOrder(QDataStream::LittleEndian);
        UbloxParser parser(currentConnection);
        /// отправить/прочитать cfg-gnss, checkBox'ы определяют флаги каждой системы в сообщении
        endata[0] = CFG::GNSS::classID; endata[1] = CFG::GNSS::messageID; endata[2] = 0x3C; endata[3] = 0x00;
        en = QByteArray(reinterpret_cast<char*>(endata), sizeof(endata));
        U1 msgVer = 0x00;
        U1 numTrkChHw = 0x00;
        U1 numTrkChUse = ui->spinGNSSUseCh->value();
        U1 numConfigBlocks = 0x07;
        stream << msgVer << numTrkChHw << numTrkChUse << numConfigBlocks;
        for (int i = 0; i<numConfigBlocks; i++){
            U1 gnssId = i;
            QGridLayout* layout = qobject_cast<QGridLayout*>(ui->frameGNSS->layout());
            int row = gnssId + 1;
            int column = 2;
            U1 resTrkCh = qobject_cast<QSpinBox*>(layout->itemAtPosition(row,column+2)->widget())->value();
            U1 maxTrkCh = qobject_cast<QSpinBox*>(layout->itemAtPosition(row,column+3)->widget())->value();
            U1 reserved = 0x00;
            bool enabled;
            U4 sigCfgMask;
            bool L1C;
            bool L2C;
            bool E1;
            bool E5;
            bool B1;
            bool B2;
            bool L1;
            bool L1S;
            bool L2;
            switch (i) {
            case 0x00:
                L1C = (bool)ui->comboRecieverSystems->itemCheckState(0);
                L2C = (bool)ui->comboRecieverSystems->itemCheckState(1);
                sigCfgMask = L1C + (L2C << 4);
                break;
            case 0x01:
                L1C = (bool)ui->comboRecieverSystems->itemCheckState(2);
                sigCfgMask = L1C;
                break;
            case 0x02:
                E1 = (bool)ui->comboRecieverSystems->itemCheckState(3);
                E5 = (bool)ui->comboRecieverSystems->itemCheckState(4);
                sigCfgMask = E1 + (E5 << 5);
                break;
            case 0x03:
                B1 = (bool)ui->comboRecieverSystems->itemCheckState(5);
                B2 = (bool)ui->comboRecieverSystems->itemCheckState(6);
                sigCfgMask = B1 + (B2 << 4);

                break;
            case 0x04:
                L1 = (bool)ui->comboRecieverSystems->itemCheckState(7);
                sigCfgMask = L1;
                break;
            case 0x05:
                L1C = (bool)ui->comboRecieverSystems->itemCheckState(8);
                L1S = (bool)ui->comboRecieverSystems->itemCheckState(9);
                L2C = (bool)ui->comboRecieverSystems->itemCheckState(10);
                sigCfgMask = L1C + (L1S << 2) + (L2C << 4);
                break;
            case 0x06:
                L1 = (bool)ui->comboRecieverSystems->itemCheckState(11);
                L2 = (bool)ui->comboRecieverSystems->itemCheckState(12);
                sigCfgMask = L1 + (L2 << 4);
                break;
            default:
                break;
            }
            enabled = sigCfgMask;
            X4 flags = enabled + (sigCfgMask << 16);
            stream << gnssId << resTrkCh << maxTrkCh << reserved << flags;
        }
        msg = hdr + en + payload;
        checkSum = UbloxParser::calcCheckSum(en + payload);
        msg.append(checkSum);
        parser.sendMessage(msg);

        /// отправить/прочитать cfg-dat, задаёт id

        /// отправить/прочитать cfg-nav5, меняет поле minElev на заданное
        /// отправить/прочитать cfg-nav5, меняет поле CNOTheshhold на заданное
        /// отправить/прочитать cfg-nav5, меняет dynMod
        endata[0] = CFG::NAV5::classID; endata[1] = CFG::NAV5::messageID; endata[2] = 0x24; endata[3] = 0x00;
        en = QByteArray(reinterpret_cast<char*>(endata), sizeof(endata));
        U2 mask = 0xffff;

        U1 dynModel = 0;
        for (int i = 0; i < ui->comboNAV5DynModel->count(); ++i) {
            QString modelName = ui->comboNAV5DynModel->itemText(i).section(" - ",1);
            if (modelName == ui->comboRecieverMode->currentText()){
                dynModel = ui->comboNAV5DynModel->itemText(i).section(' ',0,0).toUInt();
            }
        }
        if(dynModel == 0) dynModel = ui->comboNAV5DynModel->currentText().section(' ',0,0).toUInt();;
        U1 fixMode = ui->comboNAV5FixMode->currentText().section(' ',0,0).toUInt();
        I4 fixedAlt = (I4)(ui->spinNAV5FixAlt->value()*100);
        U4 fixedAltVar = (U4)(ui->spinNAV5AltVar->value()*10000);

        I1 minElev = ui->spinRecieverMinElev->value();

        U1 drLimit = ui->spinNAV5DRLimit->value();
        U2 pDop = (U2)(ui->spinNAV5PDOP->value()*10);
        U2 tDop = (U2)(ui->spinNAV5TDOP->value()*10);
        U2 pAcc = ui->spinNAV5PAcc->value();
        U2 tAcc = ui->spinNAV5TAcc->value();
        U1 staticHoldThresh = (U1)(ui->spinNAV5SHT->value()*100);
        U1 dgnssTimeout = ui->spinNAV5DGNSSTimeout->value();
        U1 cnoThreshNumSVs = ui->spinNAV5CNOThresh->value();

        U1 cnoThresh = ui->spinRecieverCNOThresh->value();

        U1 reserved1 = 0x00;
        U1 reserved2 = 0x00;
        U2 staticHoldMaxDist = ui->spinNAV5MaxDist->value();
        U1 utcStandard = ui->comboNAV5UTC->currentText().section(' ',0,0).toUInt();;
        U1 reserved3 = 0x00;
        U1 reserved4 = 0x00;
        U1 reserved5 = 0x00;
        U1 reserved6 = 0x00;
        U1 reserved7 = 0x00;
        payload.clear(); payload.resize(24);
        QDataStream nav5stream(&payload, QIODevice::WriteOnly);
        nav5stream.setByteOrder(QDataStream::LittleEndian);
        nav5stream << mask << dynModel << fixMode << fixedAlt << fixedAltVar << minElev << drLimit << pDop << tDop << pAcc << tAcc
               << staticHoldThresh << dgnssTimeout << cnoThreshNumSVs << cnoThresh << reserved1 << reserved2 << staticHoldMaxDist
               << utcStandard << reserved3 << reserved4 << reserved5 << reserved6 << reserved7;
        msg = hdr + en + payload;
        checkSum = UbloxParser::calcCheckSum(en + payload);
        msg.append(checkSum);
        parser.sendMessage(msg);
        /// отправить/прочитать cfg-dgnss diff mode соответственно
        U1 dgnssMode = (U1)ui->checkRecieverRTKSolution->isChecked() + 2;
        endata[0] = CFG::DGNSS::classID; endata[1] = CFG::DGNSS::messageID; endata[2] = 0x04; endata[3] = 0x00;
        en = QByteArray(reinterpret_cast<char*>(endata), sizeof(endata));
        U1 payloadData[] = {dgnssMode, 0x00, 0x00, 0x00};
        payload = QByteArray(reinterpret_cast<char*>(payloadData), sizeof(payloadData));
        msg = hdr + en + payload;
        checkSum = UbloxParser::calcCheckSum(en + payload);
        msg.append(checkSum);
        parser.sendMessage(msg);

        /// отправить/прочитать cfg-itfm
        endata[0] = CFG::ITFM::classID; endata[1] = CFG::ITFM::messageID; endata[2] = 0x08; endata[3] = 0x00;
        en = QByteArray(reinterpret_cast<char*>(endata), sizeof(endata));
        X4 config;
        X4 config2;
        U1 bbThreshold = ui->spinITFMbbThresh->value();
        U1 cwTheshhold = ui->spinITFMcwThresh->value();
        bool en1 = ui->checkRecieverJamming->checkState();
        U1 antType = ui->comboITFMantType->currentIndex();
        U4 algorithmBits =  0x16B156;
        U2 genetalBits = 0x31E;
        bool en2 = ui->comboITFMantType->currentIndex();
        config = bbThreshold + (cwTheshhold << 4) + (algorithmBits << 9) + (en1 << 31);
        config2 = genetalBits + (antType << 12) + (en2 << 14);
        payload.clear(); payload.resize(8);
        QDataStream itfmstream(&payload, QIODevice::WriteOnly);
        itfmstream.setByteOrder(QDataStream::LittleEndian);
        itfmstream << config << config2;
        msg = hdr + en + payload;
        checkSum = UbloxParser::calcCheckSum(en + payload);
        msg.append(checkSum);
        parser.sendMessage(msg);
        /// отправить/прочитать cfg-msg rxm-sfrbx, rxm-rawx, nav-velecef, nav-velned, nav-posecef, nav-posned, nav-relposned
        QSpinBox* spinI2C;
        QSpinBox* spinUART1;
        QSpinBox* spinUART2;
        QSpinBox* spinUSB;
        QSpinBox* spinSPI;
        en.resize(4);
        payload.resize(8);
        payload[7] = 0x00;
        en[0] = CFG::MSG::classID;
        en[1] = CFG::MSG::messageID;
        en[2] = 0x08;
        en[3] = 0x00;
        if (ui->checkRecieverVel->isChecked()){
            if (ui->checkRecieverVelECEF->isChecked()){
                spinI2C     =   ui->spinRecieverVel1;
                spinUART1   =   ui->spinRecieverVel2;
                spinUART2   =   ui->spinRecieverVel3;
                spinUSB     =   ui->spinRecieverVel4;
                spinSPI     =   ui->spinRecieverVel5;
                payload[0] = NAV::VELECEF::classID;
                payload[1] = NAV::VELECEF::messageID;
                payload[2] = (uint8_t)spinI2C->value();
                payload[3] = (uint8_t)spinUART1->value();
                payload[4] = (uint8_t)spinUART2->value();
                payload[5] = (uint8_t)spinUSB->value();
                payload[6] = (uint8_t)spinSPI->value();
                msg = hdr + en + payload;
                QByteArray checkSum = UbloxParser::calcCheckSum(en + payload);
                msg.append(checkSum);
                parser.sendMessage(msg);
            }
            else{
                payload[0] = NAV::VELECEF::classID;
                payload[1] = NAV::VELECEF::messageID;
                payload[2] = 0x00;
                payload[3] = 0x00;
                payload[4] = 0x00;
                payload[5] = 0x00;
                payload[6] = 0x00;
                msg = hdr + en + payload;
                QByteArray checkSum = UbloxParser::calcCheckSum(en + payload);
                msg.append(checkSum);
                parser.sendMessage(msg);
            }
            if (ui->checkRecieverVelNED->isChecked()){
                spinI2C     =   ui->spinRecieverVel1;
                spinUART1   =   ui->spinRecieverVel2;
                spinUART2   =   ui->spinRecieverVel3;
                spinUSB     =   ui->spinRecieverVel4;
                spinSPI     =   ui->spinRecieverVel5;
                payload[0] = NAV::VELNED::classID;
                payload[1] = NAV::VELNED::messageID;
                payload[2] = (uint8_t)spinI2C->value();
                payload[3] = (uint8_t)spinUART1->value();
                payload[4] = (uint8_t)spinUART2->value();
                payload[5] = (uint8_t)spinUSB->value();
                payload[6] = (uint8_t)spinSPI->value();
                msg = hdr + en + payload;
                QByteArray checkSum = UbloxParser::calcCheckSum(en + payload);
                msg.append(checkSum);
                parser.sendMessage(msg);
            }
            else{
                payload[0] = NAV::VELNED::classID;
                payload[1] = NAV::VELNED::messageID;
                payload[2] = 0x00;
                payload[3] = 0x00;
                payload[4] = 0x00;
                payload[5] = 0x00;
                payload[6] = 0x00;
                msg = hdr + en + payload;
                QByteArray checkSum = UbloxParser::calcCheckSum(en + payload);
                msg.append(checkSum);
                parser.sendMessage(msg);
            }
        }
        else{
            QByteArray checkSum;
            payload[0] = NAV::VELNED::classID;
            payload[1] = NAV::VELNED::messageID;
            payload[2] = 0x00;
            payload[3] = 0x00;
            payload[4] = 0x00;
            payload[5] = 0x00;
            payload[6] = 0x00;
            msg = hdr + en + payload;
            checkSum = UbloxParser::calcCheckSum(en + payload);
            msg.append(checkSum);
            parser.sendMessage(msg);
            payload[0] = NAV::VELECEF::classID;
            payload[1] = NAV::VELECEF::messageID;
            payload[2] = 0x00;
            payload[3] = 0x00;
            payload[4] = 0x00;
            payload[5] = 0x00;
            payload[6] = 0x00;
            msg = hdr + en + payload;
            checkSum = UbloxParser::calcCheckSum(en + payload);
            msg.append(checkSum);
            parser.sendMessage(msg);
        }
        if (ui->checkRecieverPos->isChecked()){
            if (ui->checkRecieverPosECEF->isChecked()){
                spinI2C     =   ui->spinRecieverPos1;
                spinUART1   =   ui->spinRecieverPos2;
                spinUART2   =   ui->spinRecieverPos3;
                spinUSB     =   ui->spinRecieverPos4;
                spinSPI     =   ui->spinRecieverPos5;
                payload[0] = NAV::POSECEF::classID;
                payload[1] = NAV::POSECEF::messageID;
                payload[2] = (uint8_t)spinI2C->value();
                payload[3] = (uint8_t)spinUART1->value();
                payload[4] = (uint8_t)spinUART2->value();
                payload[5] = (uint8_t)spinUSB->value();
                payload[6] = (uint8_t)spinSPI->value();
                msg = hdr + en + payload;
                QByteArray checkSum = UbloxParser::calcCheckSum(en + payload);
                msg.append(checkSum);
                parser.sendMessage(msg);
            }
            else{
                payload[0] = NAV::POSECEF::classID;
                payload[1] = NAV::POSECEF::messageID;
                payload[2] = 0x00;
                payload[3] = 0x00;
                payload[4] = 0x00;
                payload[5] = 0x00;
                payload[6] = 0x00;
                msg = hdr + en + payload;
                QByteArray checkSum = UbloxParser::calcCheckSum(en + payload);
                msg.append(checkSum);
                parser.sendMessage(msg);
            }
            if (ui->checkRecieverPosLLH->isChecked()){
                spinI2C     =   ui->spinRecieverPos1;
                spinUART1   =   ui->spinRecieverPos2;
                spinUART2   =   ui->spinRecieverPos3;
                spinUSB     =   ui->spinRecieverPos4;
                spinSPI     =   ui->spinRecieverPos5;
                payload[0] = NAV::POSLLH::classID;
                payload[1] = NAV::POSLLH::messageID;
                payload[2] = (uint8_t)spinI2C->value();
                payload[3] = (uint8_t)spinUART1->value();
                payload[4] = (uint8_t)spinUART2->value();
                payload[5] = (uint8_t)spinUSB->value();
                payload[6] = (uint8_t)spinSPI->value();
                msg = hdr + en + payload;
                QByteArray checkSum = UbloxParser::calcCheckSum(en + payload);
                msg.append(checkSum);
                parser.sendMessage(msg);
            }
            else{
                payload[0] = NAV::POSLLH::classID;
                payload[1] = NAV::POSLLH::messageID;
                payload[2] = 0x00;
                payload[3] = 0x00;
                payload[4] = 0x00;
                payload[5] = 0x00;
                payload[6] = 0x00;
                msg = hdr + en + payload;
                QByteArray checkSum = UbloxParser::calcCheckSum(en + payload);
                msg.append(checkSum);
                parser.sendMessage(msg);
            }
        }
        else{
            QByteArray checkSum;
            payload[0] = NAV::POSECEF::classID;
            payload[1] = NAV::POSECEF::messageID;
            payload[2] = 0x00;
            payload[3] = 0x00;
            payload[4] = 0x00;
            payload[5] = 0x00;
            payload[6] = 0x00;
            msg = hdr + en + payload;
            checkSum = UbloxParser::calcCheckSum(en + payload);
            msg.append(checkSum);
            parser.sendMessage(msg);
            payload[0] = NAV::POSLLH::classID;
            payload[1] = NAV::POSLLH::messageID;
            payload[2] = 0x00;
            payload[3] = 0x00;
            payload[4] = 0x00;
            payload[5] = 0x00;
            payload[6] = 0x00;
            msg = hdr + en + payload;
            checkSum = UbloxParser::calcCheckSum(en + payload);
            msg.append(checkSum);
            parser.sendMessage(msg);
        }
        if (ui->checkRecieverRelpos->isChecked()){
            spinI2C     =   ui->spinRecieverRelPos1;
            spinUART1   =   ui->spinRecieverRelPos2;
            spinUART2   =   ui->spinRecieverRelPos3;
            spinUSB     =   ui->spinRecieverRelPos4;
            spinSPI     =   ui->spinRecieverRelPos5;
            payload[0] = NAV::RELPOSNED::classID;
            payload[1] = NAV::RELPOSNED::messageID;
            payload[2] = (uint8_t)spinI2C->value();
            payload[3] = (uint8_t)spinUART1->value();
            payload[4] = (uint8_t)spinUART2->value();
            payload[5] = (uint8_t)spinUSB->value();
            payload[6] = (uint8_t)spinSPI->value();
            msg = hdr + en + payload;
            QByteArray checkSum = UbloxParser::calcCheckSum(en + payload);
            msg.append(checkSum);
            parser.sendMessage(msg);
        }
        else{
            payload[0] = NAV::RELPOSNED::classID;
            payload[1] = NAV::RELPOSNED::messageID;
            payload[2] = 0x00;
            payload[3] = 0x00;
            payload[4] = 0x00;
            payload[5] = 0x00;
            payload[6] = 0x00;
            msg = hdr + en + payload;
            QByteArray checkSum = UbloxParser::calcCheckSum(en + payload);
            msg.append(checkSum);
            parser.sendMessage(msg);
        }
        if (ui->checkRecieverRaw->isChecked()){
            spinI2C     =   ui->spinRecieverRaw1;
            spinUART1   =   ui->spinRecieverRaw2;
            spinUART2   =   ui->spinRecieverRaw3;
            spinUSB     =   ui->spinRecieverRaw4;
            spinSPI     =   ui->spinRecieverRaw5;
            payload[0] = RXM::RAWX::classID;
            payload[1] = RXM::RAWX::messageID;
            payload[2] = (uint8_t)spinI2C->value();
            payload[3] = (uint8_t)spinUART1->value();
            payload[4] = (uint8_t)spinUART2->value();
            payload[5] = (uint8_t)spinUSB->value();
            payload[6] = (uint8_t)spinSPI->value();
            msg = hdr + en + payload;
            QByteArray checkSum = UbloxParser::calcCheckSum(en + payload);
            msg.append(checkSum);
            parser.sendMessage(msg);
        }
        else{
            payload[0] = RXM::RAWX::classID;
            payload[1] = RXM::RAWX::messageID;
            payload[2] = 0x00;
            payload[3] = 0x00;
            payload[4] = 0x00;
            payload[5] = 0x00;
            payload[6] = 0x00;
            msg = hdr + en + payload;
            QByteArray checkSum = UbloxParser::calcCheckSum(en + payload);
            msg.append(checkSum);
            parser.sendMessage(msg);
        }
        if (ui->checkRecieverEph->isChecked()){
            spinI2C     =   ui->spinRecieverEph1;
            spinUART1   =   ui->spinRecieverEph2;
            spinUART2   =   ui->spinRecieverEph3;
            spinUSB     =   ui->spinRecieverEph4;
            spinSPI     =   ui->spinRecieverEph5;
            payload[0] = RXM::SFRBX::classID;
            payload[1] = RXM::SFRBX::messageID;
            payload[2] = (uint8_t)spinI2C->value();
            payload[3] = (uint8_t)spinUART1->value();
            payload[4] = (uint8_t)spinUART2->value();
            payload[5] = (uint8_t)spinUSB->value();
            payload[6] = (uint8_t)spinSPI->value();
            msg = hdr + en + payload;
            QByteArray checkSum = UbloxParser::calcCheckSum(en + payload);
            msg.append(checkSum);
            parser.sendMessage(msg);
        }
        else{
            payload[0] = RXM::SFRBX::classID;
            payload[1] = RXM::SFRBX::messageID;
            payload[2] = 0x00;
            payload[3] = 0x00;
            payload[4] = 0x00;
            payload[5] = 0x00;
            payload[6] = 0x00;
            msg = hdr + en + payload;
            QByteArray checkSum = UbloxParser::calcCheckSum(en + payload);
            msg.append(checkSum);
            parser.sendMessage(msg);
        }
    }
    else if (protocol == "Unicore"){
        UnicoreParser parser(currentConnection);
        /// GNSS
        QMap<QString, QString> freqMap;
        freqMap["GPS L1C/A"] = "L1C/A";
        freqMap["GPS L2C"] = "L2C";
        freqMap["Galileo E1"] = "E1";
        freqMap["Galileo E5b"] = "E5b";
        freqMap["BeiDou B1I"] = "B1";
        freqMap["BeiDou B2I"] = "B2";
        freqMap["QZSS L1C/A"] = "Q1CA";
        freqMap["QZSS L2C"] = "Q2C";
        freqMap["GLONASS L1"] = "R1";
        freqMap["GLONASS L2"] = "R2";
        QStringList systemsList = {"GPS", "BDS", "GLO", "GAL", "QZSS"};
        foreach (QString system, systemsList) {
            parser.sendMessage("UNMASK " + system);
        }

        for (int i = 0; i<ui->comboRecieverSystems->count(); i++){
            QString freq = freqMap[ui->comboRecieverSystems->itemText(i)];
            if (freq.isEmpty()) continue;
            if (ui->comboRecieverSystems->itemCheckState(i) == Qt::Checked) parser.sendMessage("MASK " + freq);
            else parser.sendMessage("UNMASK " + freq);
        }
        ui->comboRecieverSystems->uncheckAll();
        /// min elev
        foreach (QString system, systemsList) {
            parser.sendMessage("MASK " + QString::number(ui->spinRecieverMinElev->value()) + " " + system);
        }

        /// CN0
        parser.sendMessage("MASK CN0 " + QString::number(ui->spinRecieverCNOThresh->value()));



        /// RTK
        if (ui->checkRecieverRTKSolution->isChecked()){
            parser.sendMessage("CONFIG RTK USER_DEFAULTS");
            parser.sendMessage("CONFIG DGPS TIMEOUT 300");
        }
        else{
            parser.sendMessage("CONFIG RTK DISABLE");
            parser.sendMessage("CONFIG DGPS TIMEOUT 0");
        }

        /// ANTIJAM
        if (ui->checkRecieverJamming->isChecked()) parser.sendMessage("CONFIG ANTIJAM FORCE");
        else parser.sendMessage("CONFIG ANTIJAM DISABLE");

        /// eph
        if (ui->checkRecieverEph->isChecked()){
            parser.sendMessage("GPSEPHA " + QString::number(ui->spinRecieverEph3->value()));
            parser.sendMessage("BD3EPHA " + QString::number(ui->spinRecieverEph3->value()));
            parser.sendMessage("GLOEPHA " + QString::number(ui->spinRecieverEph3->value()));
            parser.sendMessage("GALEPHA " + QString::number(ui->spinRecieverEph3->value()));
        }
        else{
            parser.sendMessage("UNLOG GPSEPHA");
            parser.sendMessage("UNLOG BD3EPHA");
            parser.sendMessage("UNLOG GLOEPHA");
            parser.sendMessage("UNLOG GALEPHA");
        }

        /// RAW
        if (ui->checkRecieverRaw->isChecked()) parser.sendMessage("OBSVMA " + QString::number(ui->spinRecieverRaw3->value()));
        else parser.sendMessage("UNLOG OBSVMA");

        /// vel pos
        if (ui->checkRecieverPos->isChecked() || ui->checkRecieverVel->isChecked()){
            if (ui->checkRecieverPos->isChecked()) {
                if (ui->checkRecieverPosLLH->isChecked()) parser.sendMessage("BESTNAVA " + QString::number(ui->spinRecieverPos3->value()));
                if (ui->checkRecieverPosECEF->isChecked()) parser.sendMessage("BESTNAVXYZA " + QString::number(ui->spinRecieverPos3->value()));
            }
            if (ui->checkRecieverVel->isChecked()){
                if (ui->checkRecieverVelNED->isChecked()) parser.sendMessage("BESTNAVA " + QString::number(ui->spinRecieverVel3->value()));
                if (ui->checkRecieverVelECEF->isChecked()) parser.sendMessage("BESTNAVXYZA " + QString::number(ui->spinRecieverVel3->value()));
            }
        }
        else {
            parser.sendMessage("UNLOG BESTNAVA");
            parser.sendMessage("UNLOG BESTNAVXYZA");
        }

        /// mode
        QMap<QString,QString> modeMap;
        modeMap["Portable"] = "ROVER";
        modeMap["Stationary"] = "BASE";
        modeMap["Pedestrian"] = "ROVER SURVEY";
        modeMap["Automotive"] = "ROVER AUTOMOTIVE";
        modeMap["Sea"] = "";
        modeMap["uav"] = "ROVER UAV";
        modeMap["wrist worn watch"] = "";
        modeMap["Bike"] = "ROVER AUTOMOTIVE";
        modeMap["Other"] = "";
        qDebug() << modeMap[ui->comboRecieverMode->currentText()];
        parser.sendMessage("MODE " + modeMap[ui->comboRecieverMode->currentText()]);

        /// rel pos
        if (ui->checkRecieverRelpos->isChecked()) parser.sendMessage("UNIHEADINGA " + QString::number(ui->spinRecieverRelPos3->value()));
        else parser.sendMessage("UNLOG UNIHEADINGA");
    }
    updateSettings();
}
