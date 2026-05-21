#include "experimentconfigurationdialog.h"
#include "ui_experimentconfigurationdialog.h"

/// TODO: Третья вкладка
/// Каждый параметр представляется в виде группы из трёх координат/углов
/// Нужно удалять координаты/углы не по одному, а группой
/// Скорее всего нужно будет создавать для каждого параметра по groupBox с названием параметра
/// Внутри groupBox будут параметры и ещё ниже кнопка удаления группы
/// TODO: Сохранение в файл
/// может быть всё-таки разобраться с подключением yaml-cpp

namespace YAML {
template<>
struct convert<QString> {
    static Node encode(const QString& rhs) {
        Node node;
        node = rhs.toStdString();
        return node;
    }

    static bool decode(const Node& node, QString& rhs) {
        rhs = QString::fromStdString(node.as<std::string>());
        return true;
    }
};
}

// общее

void experimentConfigurationDialog::saveAll()
{
    saveAboutExperiment();
    saveConnections();
    saveMounting();
}

void experimentConfigurationDialog::saveAboutExperiment(){
    QFile file(aboutExperimentDirectory);
    file.open(QIODevice::WriteOnly | QIODevice::Truncate);
    file.close();
    YAML::Node config = YAML::LoadFile(aboutExperimentDirectory.toStdString());
    config["Name"] = ui->lineEditExperimentID->text();
    config["Date"] = ui->dateEdit->date().toString("dd.MM.yyyy");
    config["Goal"] = ui->textEditGoal->toPlainText();
    config["Type"] = ui->comboBoxExperimentType->currentText();
    YAML::Node generalInfo = config["General info"];
    generalInfo.reset();
    generalInfo = config["General info"];
    if (ui->comboBoxExperimentType->currentText() == "Подвижный"){
        generalInfo["Start point"] = ui->lineEditStartPoint->text();
        generalInfo["End point"] = ui->lineEditEndPoint->text();
        generalInfo["Movement method"] = ui->lineEditMovementMethod->text();
        generalInfo["About vehicle"] = ui->lineEditAboutVehicle->text();
    }
    else {
        generalInfo["Location of event"] = ui->lineEditLocationOfEvent->text();
    }
    YAML::Node team = config["Team"];
    team.reset();
    team = config["Team"];
    for (int row = 0; row < ui->tableTeam->rowCount(); ++row) {
        QString FIO = ui->tableTeam->item(row,0)->text();
        QString role = qobject_cast<QComboBox*>(ui->tableTeam->cellWidget(row,1))->currentText();
        YAML::Node member;
        member["Name"] = FIO;
        member["Role"] = role;
        team.push_back(member);
    }
    std::ofstream fout(aboutExperimentDirectory.toStdString());
    fout << config;
}

void experimentConfigurationDialog::saveConnections(){
    QFile file(experimentConnectionsDirectory);
    file.open(QIODevice::WriteOnly | QIODevice::Truncate);
    file.close();
    YAML::Node config = YAML::LoadFile(experimentConnectionsDirectory.toStdString());
    YAML::Node Devices = config["Devices"];
    YAML::Node Connections = config["Connections"];
    foreach (auto device, this->devicesMap) {
        YAML::Node deviceNode;
        deviceNode["Name"] = device.id;
        deviceNode["Model"] = device.model;
        QString type = device.type;
        deviceNode["Type"] = type;
        YAML::Node ports;
        foreach (auto port, device.outputs) {
            ports.push_back(port);
        }
        deviceNode["Ports"] = ports;
        YAML::Node Parameters;
        if (type == "IMU"){
            Parameters["Instability bias"] = device.imuInfo.instabBias;
            Parameters["Random walk"] = device.imuInfo.randomWalk;
            Parameters["Initial error"] = device.imuInfo.initialError;
        }
        else if (type == "Антенна"){
            Parameters["Antex/PCV file path"] = device.antennaInfo.confFileDirectory;
        }
        else if (type == "Камера"){
            Parameters["fps"] = device.cameraInfo.fps;
            Parameters["Height"] = device.cameraInfo.heigt;
            Parameters["Width"] = device.cameraInfo.width;
        }
        deviceNode["Parameters"] = Parameters;
        Devices.push_back(deviceNode);
    }
    foreach (auto connection, connectionsMap) {
        YAML::Node ConnectionNode;
        ConnectionNode["Device1"] = connection.device1;
        ConnectionNode["Device2"] = connection.device2;
        ConnectionNode["Port1"] = connection.port1;
        ConnectionNode["Port2"] = connection.port2;
        QString connType = connection.connectionType;
        ConnectionNode["Connection type"] = connType;
        YAML::Node Parameters;
        if (connType == "Serial"){
            Parameters["Baudrate"] = connection.serialInfo.baudrate;
            Parameters["Parity"] = connection.serialInfo.parity;
            Parameters["Data bits"] = connection.serialInfo.dataBits;
            Parameters["Stop bits"] = connection.serialInfo.stopBits;
        }
        else if (connType == "CAN"){
            Parameters["Baudrate"] = connection.canInfo.baudrate;
        }
        else if (connType == "Коакс. кабель"){
            Parameters["Length"] = connection.coaxCableInfo.length;
            Parameters["Material"] = connection.coaxCableInfo.material;
            Parameters["Data loss"] = connection.coaxCableInfo.dataLoss;
        }
        else if (connType == "TCP"){
            Parameters["Adress"] = connection.tcpInfo.adress;
            Parameters["Port"] = connection.tcpInfo.port;
        }
        ConnectionNode["Parameters"] = Parameters;
        Connections.push_back(ConnectionNode);
    }
    std::ofstream fout(experimentConnectionsDirectory.toStdString());
    fout << config;
}

void experimentConfigurationDialog::saveMounting(){
    int count = ui->areaDeviceParameters->count();
    QFile file(experimentMountingDirectory);
    file.open(QIODevice::WriteOnly | QIODevice::Truncate);
    file.close();
    YAML::Node config = YAML::LoadFile(experimentMountingDirectory.toStdString());
    YAML::Node Mounting = config["Mounting"];
    for (int i = 0; i < count; ++i) {
        QGroupBox* group = ui->areaDeviceParameters->groupAtPosition(i);
        QString device = group->title();
        YAML::Node DeviceNode;
        DeviceNode["Name"] = device;
        QList<QGroupBox*> parameterGroups = getParameters(group);
        YAML::Node ParametersNode;
        foreach (QGroupBox* parameterGroup, parameterGroups) {
            QString name = parameterGroup->title();
            YAML::Node ParameterNode;
            QGridLayout* layout = qobject_cast<QGridLayout*>(parameterGroup->layout());
            for (int j = 0; j < layout->rowCount(); ++j) {
                QLabel* label = qobject_cast<QLabel*>(layout->itemAtPosition(j,0)->widget());
                QSpinBox* spin =  qobject_cast<QSpinBox*>(layout->itemAtPosition(j,1)->widget());
                QString parameterName = label->text();
                int value = spin->value();
                ParameterNode[parameterName] = value;
            }
            ParametersNode[name] = ParameterNode;
        }
        DeviceNode["Parameters"] = ParametersNode;
        Mounting.push_back(DeviceNode);
    }
    std::ofstream fout(experimentMountingDirectory.toStdString());
    fout << config;
}

void experimentConfigurationDialog::saveTab()
{
    int index = ui->tabWidget->currentIndex();
    if (index == 0){
        saveAboutExperiment();
    }
    else if (index == 1){
        saveConnections();
    }
    else if (index == 2){
        saveMounting();
    }
}

/*
Devices:
 - Name:
   Model:
   Type: camera
   Parameters:
   Ports:
    - o1
    - o2
    - o3
 - Name:
...
Connections:
 - Device1:
   Device2:
   Port1:
   Port2:
   Connection type:
   Connection parameters:
    Bitrate:
    Start bits:
    Stop bits:
 - Device1:
...
Mounting:
 - Name:
   Parameters:
    Position:
     - 0.0
     - 0.0
     - 0.0
    Boresight: []
    Lever arm: []


*/
void experimentConfigurationDialog::setupTableSize(QTableWidget* table) {
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

void experimentConfigurationDialog::addSaveButton(int index)
{
    QPushButton* button = new QPushButton("");
    button->setStyleSheet("image: url(:/resources/save.png);");

    connect(button, &QPushButton::clicked, this, &experimentConfigurationDialog::saveTab);
    ui->tabWidget->tabBar()->setTabButton(index, QTabBar::RightSide, button);
}

experimentConfigurationDialog::experimentConfigurationDialog(QString experimentDirectory,QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::experimentConfigurationDialog)
{
    ui->setupUi(this);
    this->currentDirectory = experimentDirectory + '/' + "Configurations" + '/' + "Experiment_configurations";
    this->aboutExperimentDirectory = currentDirectory + '/' + "About_experiment.yaml";
    this->experimentConnectionsDirectory = currentDirectory + '/' + "Experiment_connections.yaml";
    this->experimentMountingDirectory = currentDirectory + '/' + "Experiment_mounting.yaml";
    QFile aboutFile(aboutExperimentDirectory);
    if (aboutFile.open(QIODevice::ReadOnly)){
        aboutFile.close();
        YAML::Node config = YAML::LoadFile(aboutExperimentDirectory.toStdString());
        if (config["Name"]) ui->lineEditExperimentID->setText(config["Name"].as<QString>());
        if (config["Date"]) ui->dateEdit->setDate(QDate::fromString(config["Date"].as<QString>(),"dd.MM.yyyy"));
        else ui->dateEdit->setDate(QDate::currentDate());
        if (config["Goal"]) ui->textEditGoal->setText(config["Goal"].as<QString>());
        if (config["Type"]) {
            QString type = config["Type"].as<QString>();
            ui->comboBoxExperimentType->setCurrentText(type);
            YAML::Node generalInfo = config["General info"];
            if (type == "Подвижный"){
                if (generalInfo["Start point"]) ui->lineEditStartPoint->setText(generalInfo["Start point"].as<QString>());
                if (generalInfo["End point"]) ui->lineEditEndPoint->setText(generalInfo["End point"].as<QString>());
                if (generalInfo["Movement method"]) ui->lineEditMovementMethod->setText(generalInfo["Movement method"].as<QString>());
                if (generalInfo["About vehicle"]) ui->lineEditAboutVehicle->setText(generalInfo["About vehicle"].as<QString>());
                ui->frameStationary->setHidden(true);
            }
            else {
                if (generalInfo["Location of event"]) ui->lineEditLocationOfEvent->setText(generalInfo["Location of event"].as<QString>());
                ui->frameMoving->setHidden(true);
            }
        }
        YAML::Node team = config["Team"];
        for (const YAML::Node& op : team) {
            int row = ui->tableTeam->rowCount();
            addTeamMember();
            QString name = op["Name"] ? op["Name"].as<QString>() : "";
            QString role = op["Role"] ? op["Role"].as<QString>() : "";
            QTableWidgetItem *item = new QTableWidgetItem(name);
            ui->tableTeam->setItem(row,0,item);
            QComboBox* combo = qobject_cast<QComboBox*>(ui->tableTeam->cellWidget(row,1));
            combo->setCurrentText(role);
        }
    }
    QFile connectionsFile(experimentConnectionsDirectory);
    if (connectionsFile.open(QIODevice::ReadOnly)){
        connectionsFile.close();
        YAML::Node config = YAML::LoadFile(experimentConnectionsDirectory.toStdString());
        if (config["Devices"]) {
            YAML::Node Devices = config["Devices"];
            for (const YAML::Node& deviceNode : Devices) {
                ExperimentDeviceInfo info;
                QString id = deviceNode["Name"]? deviceNode["Name"].as<QString>() : "";
                if (id.isEmpty()) continue;
                info.id = id;
                info.model = deviceNode["Model"]? deviceNode["Model"].as<QString>(): "";
                QString type = deviceNode["Type"]? deviceNode["Type"].as<QString>(): "Приемник";
                info.type = type;
                QStringList ports;
                YAML::Node Ports = deviceNode["Ports"];
                if (Ports){
                    for (const YAML::Node& portNode : Ports) {
                        ports.append(portNode.as<QString>());
                    }
                    info.outputs = ports;
                }
                YAML::Node Parameters = deviceNode["Parameters"];
                if (Parameters){
                    if (type == "IMU"){
                        if (Parameters["Instability bias"]) info.imuInfo.instabBias = Parameters["Instability bias"].as<double>();
                        if (Parameters["Random walk"]) info.imuInfo.randomWalk = Parameters["Random walk"].as<double>();
                        if (Parameters["Initial error"]) info.imuInfo.initialError = Parameters["Initial error"].as<double>();
                    }
                    else if (type == "Антенна"){
                        if (Parameters["Antex/PCV file path"]) info.antennaInfo.confFileDirectory = Parameters["Antex/PCV file path"].as<QString>();
                    }
                    else if (type == "Камера"){
                        if (Parameters["fps"]) info.cameraInfo.fps = Parameters["fps"].as<int>();
                        if (Parameters["Height"]) info.cameraInfo.heigt = Parameters["Height"].as<int>();
                        if (Parameters["Width"]) info.cameraInfo.width = Parameters["Width"].as<int>();
                    }
                }
                devicesMap[id] = info;
                QListWidgetItem *item = new QListWidgetItem();
                item->setText(id);
                ui->listWidgetDevices->addItem(item);
            }
        }
        if (config["Connections"]){
            YAML::Node Connections = config["Connections"];
            for (const YAML::Node& connectionNode : Connections) {
                ExperimentConnectionInfo info;
                QString device1 = connectionNode["Device1"]? connectionNode["Device1"].as<QString>() : "";
                QString device2 = connectionNode["Device2"]? connectionNode["Device2"].as<QString>() : "";
                QString port1 = connectionNode["Port1"]? connectionNode["Port1"].as<QString>() : "";
                QString port2 = connectionNode["Port2"]? connectionNode["Port2"].as<QString>() : "";
                info.device1 = device1;
                info.device2 = device2;
                info.port1 = port1;
                info.port2 = port2;
                QString connType = connectionNode["Connection type"]? connectionNode["Connection type"].as<QString>() : "";
                info.connectionType = connType;
                YAML::Node Parameters = connectionNode["Parameters"];
                if (Parameters){
                    if (connType == "Serial"){
                        if (Parameters["Baudrate"])  info.serialInfo.baudrate = Parameters["Baudrate"].as<int>();
                        if (Parameters["Parity"])    info.serialInfo.parity = Parameters["Parity"].as<QString>();
                        if (Parameters["Data bits"]) info.serialInfo.dataBits = Parameters["Data bits"].as<int>();
                        if (Parameters["Stop bits"]) info.serialInfo.stopBits = Parameters["Stop bits"].as<int>();
                    }
                    else if (connType == "CAN"){
                        if (Parameters["Baudrate"]) info.canInfo.baudrate = Parameters["Baudrate"].as<int>();
                    }
                    else if (connType == "Коакс. кабель"){
                        if (Parameters["Length"]) info.coaxCableInfo.length = Parameters["Length"].as<QString>();
                        if (Parameters["Material"]) info.coaxCableInfo.material = Parameters["Material"].as<QString>();
                        if (Parameters["Data loss"]) info.coaxCableInfo.dataLoss = Parameters["Data loss"].as<int>();
                    }
                    else if (connType == "TCP"){
                        if (Parameters["Adress"]) info.tcpInfo.adress = Parameters["Adress"].as<QString>();
                        if (Parameters["Port"]) info.tcpInfo.port = Parameters["Port"].as<QString>();
                    }
                }
                QTableWidgetItem* device1Item = new QTableWidgetItem(device1);
                QTableWidgetItem* device2Item = new QTableWidgetItem(device2);
                QTableWidgetItem* device1OutputItem = new QTableWidgetItem(port1);
                QTableWidgetItem* device2OutputItem = new QTableWidgetItem(port2);
                QTableWidgetItem* connectionTypeItem = new QTableWidgetItem(connType);
                connectionsMap[device1 + '&' + device2] = info;
                int row = ui->tableWidgetConnections->rowCount();
                ui->tableWidgetConnections->setRowCount(row + 1);
                ui->tableWidgetConnections->setItem(row, 0, device1Item);
                ui->tableWidgetConnections->setItem(row, 1, device1OutputItem);
                ui->tableWidgetConnections->setItem(row, 2, connectionTypeItem);
                ui->tableWidgetConnections->setItem(row, 3, device2Item);
                ui->tableWidgetConnections->setItem(row, 4, device2OutputItem);
                setupTableSize(ui->tableWidgetConnections);
            }
        }
    }
    QFile mountingFile(experimentMountingDirectory);
    if (mountingFile.open(QIODevice::ReadOnly)){
        mountingFile.close();
        YAML::Node config = YAML::LoadFile(experimentMountingDirectory.toStdString());
        if (config["Mounting"]) {
            YAML::Node Mounting = config["Mounting"];
            for (const auto& Mount : Mounting) {
                QString device = Mount["Name"]? Mount["Name"].as<QString>() : "";
                if (device.isEmpty()) continue;
                QGroupBox* parametersGroup = createParametersGroup(device,false);
                ExperimentGroupBoxParametersInfo* info = new ExperimentGroupBoxParametersInfo;
                info->boresightCheck = false;
                info->leverarmCheck = false;
                info->positionCheck = false;
                info->deviceName = device;
                info->group = parametersGroup;
                if (Mount["Parameters"]){
                    YAML::Node Parameters = Mount["Parameters"];
                    for (const auto& Parameter : Parameters) {
                        QString parameterName = Parameter.first.as<QString>();
                        QGroupBox* parameterGroup = addParameter(parameterName,parametersGroup);
                        int index = 0;
                        QList<QPair<bool*, QString>> parametersKeysList = {
                            qMakePair(&info->positionCheck, QString("Позиция")),
                            qMakePair(&info->boresightCheck, QString("Ошибка угла наведения")),
                            qMakePair(&info->leverarmCheck, QString("Lever arm"))
                        };
                        foreach (auto pair, parametersKeysList) {
                            if (pair.second == parameterName) {
                                *pair.first = true;
                            }
                        }
                        for (const auto& Field : Parameter.second) {
                            int value = Field.second.as<int>();
                            QGridLayout* layout = qobject_cast<QGridLayout*>(parameterGroup->layout());
                            if (!layout) continue;
                            QSpinBox* spin = qobject_cast<QSpinBox*>(layout->itemAtPosition(index,1)->widget());
                            if (!spin) continue;
                            spin->setValue(value);
                            index++;
                        }
                    }
                }
                int index = ui->areaDeviceParameters->addGroup(parametersGroup);
                info->row = index/2;
                info->column = index%2;
                groupBoxParametersMap[device] = *info;
            }
        }
    }
    addSaveButton(0);
    addSaveButton(1);
    addSaveButton(2);
    ui->tabWidget->tabBar()->tabButton(1,QTabBar::RightSide)->setHidden(true);
    ui->tabWidget->tabBar()->tabButton(2,QTabBar::RightSide)->setHidden(true);

}

experimentConfigurationDialog::~experimentConfigurationDialog()
{
    delete ui;
}

void experimentConfigurationDialog::changeSaveButtons(int index){
    ui->tabWidget->tabBar()->tabButton(index,QTabBar::RightSide)->setHidden(false);
    QList<int> indexList = {0,1,2};
    indexList.removeOne(index);
    foreach (int index, indexList) {
        ui->tabWidget->tabBar()->tabButton(index,QTabBar::RightSide)->setHidden(true);
    }
}

// общее

// 1я вкладка

void experimentConfigurationDialog::addTeamMember()
{
    int row = ui->tableTeam->rowCount();
    ui->tableTeam->setRowCount(row+1);
    QComboBox* combo = new QComboBox(ui->tableTeam);
    combo->addItems({"организатор", "регистратор", "наблюдатель", "водитель"});
    ui->tableTeam->setCellWidget(row,1,combo);
}

void experimentConfigurationDialog::deleteTeamMember()
{
    if (ui->tableTeam->selectedItems().count() == 1){
        int row = ui->tableTeam->currentItem()->row();
        delete ui->tableTeam->cellWidget(row,1);
        ui->tableTeam->removeRow(ui->tableTeam->currentRow());
    }
}

void experimentConfigurationDialog::changeGeneralInfo(QString experimentType)
{
    ui->groupBoxGeneralInfo->setEnabled(true);
    foreach (auto object, ui->groupBoxGeneralInfo->children()) {
        if(!qobject_cast<QVBoxLayout*>(object)){
            QWidget *widget = qobject_cast<QWidget*>(object);
            widget->setHidden(true);
        }
    }
    if (experimentType == "Стационарный"){
        ui->frameStationary->setHidden(false);
    }
    else{
        ui->frameMoving->setHidden(false);
    }
}

// 1я вкладка

// 2я вкладка
void experimentConfigurationDialog::changeGroupBoxInfo(QGroupBox* groupInfo)
{
    QComboBox *comboType = qobject_cast<QComboBox*>(sender());
    QGridLayout *groupLayoutOld = qobject_cast<QGridLayout*>(groupInfo->layout());
    foreach (auto object, groupInfo->children()) {
        if(!qobject_cast<QGridLayout*>(object)){
            QWidget *widget = qobject_cast<QWidget*>(object);
            groupLayoutOld->removeWidget(widget);
            delete widget;
        }
    }
    delete groupLayoutOld;
    QGridLayout *groupLayoutNew = new QGridLayout();
    groupInfo->setLayout(groupLayoutNew);
    QString type = comboType->currentText();
    if (type == "IMU"){
        QLabel *labelShiftDistability = new QLabel("Нестабильность смещения: ");
        QDoubleSpinBox *lineShiftDistability = new QDoubleSpinBox();
        QLabel *labelRandomLuft = new QLabel("Случайное блуждание: ");
        QDoubleSpinBox *lineRandomLuft = new QDoubleSpinBox();
        QLabel *labelStartError = new QLabel("Начальная погрешность: ");
        QDoubleSpinBox *lineStartError = new QDoubleSpinBox();

        groupLayoutNew->addWidget(labelShiftDistability, 0, 0);
        groupLayoutNew->addWidget(lineShiftDistability, 0, 1);
        groupLayoutNew->addWidget(labelRandomLuft, 1, 0);
        groupLayoutNew->addWidget(lineRandomLuft, 1, 1);
        groupLayoutNew->addWidget(labelStartError, 2, 0);
        groupLayoutNew->addWidget(lineStartError, 2, 1);
    }
    else if (type == "Камера"){
        QLabel *labelFPS = new QLabel("Кол-во кадров в секунду: ");
        QSpinBox *lineFPS = new QSpinBox();
        QLabel *labelHeight = new QLabel("Высота: ");
        QSpinBox *lineHeight = new QSpinBox();
        QLabel *labelWidth = new QLabel("Ширина: ");
        QSpinBox *lineWidth = new QSpinBox();

        groupLayoutNew->addWidget(labelFPS, 0, 0);
        groupLayoutNew->addWidget(lineFPS, 0, 1);
        groupLayoutNew->addWidget(labelHeight, 1, 0);
        groupLayoutNew->addWidget(lineHeight, 1, 1);
        groupLayoutNew->addWidget(labelWidth, 2, 0);
        groupLayoutNew->addWidget(lineWidth, 2, 1);

    }
    else if (type == "Антенна"){
        QLabel *labelDirectory = new QLabel("Путь к Antex/PCV файлу: ");
        QLineEdit *lineDirectory = new QLineEdit();

        groupLayoutNew->addWidget(labelDirectory, 0, 0);
        groupLayoutNew->addWidget(lineDirectory, 0, 1);
    }
}

void experimentConfigurationDialog::addOutput(QListWidget *listOutputs)
{
    QListWidgetItem *item = new QListWidgetItem("Ведите название выхода");
    item->setFlags(Qt::ItemIsEnabled | Qt::ItemIsSelectable | Qt::ItemIsEditable);
    listOutputs->addItem(item);
}

void experimentConfigurationDialog::deleteOutput(QListWidget *listOutputs)
{
    if (!listOutputs->selectedItems().isEmpty()){
        QListWidgetItem *item = listOutputs->takeItem(listOutputs->currentRow());
        delete item;
    }
}

void experimentConfigurationDialog::addDevice()
{
    QDialog dialog(this);
    dialog.setWindowTitle("Редактор устройств");
    QVBoxLayout *layout = new QVBoxLayout(&dialog);
    QHBoxLayout *layoutLines = new QHBoxLayout();
    QHBoxLayout *layoutGroupBoxes = new QHBoxLayout();
    QHBoxLayout *hLayout = new QHBoxLayout();
    QLabel *labelID = new QLabel("ID: ");
    QLineEdit *lineID = new QLineEdit();
    QLabel *labelModel = new QLabel("Модель: ");
    QLineEdit *lineModel = new QLineEdit();
    QLabel *labelType = new QLabel("Тип: ");
    QComboBox *comboType = new QComboBox();
    comboType->addItems({"Приёмник", "IMU", "Камера", "Одометр", "Антенна", "Сплиттер", "Другое"});
    comboType->setCurrentIndex(-1);
    QGroupBox *groupOutputs = new QGroupBox("Выводы");
    groupOutputs->setAlignment(Qt::AlignHCenter);
    QGroupBox *groupInfo = new QGroupBox("Дополнительная информация");
    groupInfo->setAlignment(Qt::AlignHCenter);
    QVBoxLayout *groupOutputsLayout = new QVBoxLayout();
    QGridLayout *groupInfoLayout = new QGridLayout();
    QListWidget *listOutputs = new QListWidget();
    QVBoxLayout *layoutOutputButtons = new QVBoxLayout();
    QPushButton *addButton = new QPushButton("");
    addButton->setStyleSheet("image: url(:/resources/add.png);");
    QPushButton *deleteButton = new QPushButton("");
    deleteButton->setStyleSheet("image: url(:/resources/delete.png);");

    QPushButton *okButton = new QPushButton("Готово");
    QPushButton *cancelButton = new QPushButton("Отмена");

    layoutLines->addWidget(labelID);
    layoutLines->addWidget(lineID);
    layoutLines->addWidget(labelModel);
    layoutLines->addWidget(lineModel);
    layoutLines->addWidget(labelType);
    layoutLines->addWidget(comboType);

    groupOutputs->setLayout(groupOutputsLayout);
    groupOutputs->layout()->addWidget(listOutputs);
    groupInfo->setLayout(groupInfoLayout);
    layoutGroupBoxes->addWidget(groupOutputs);
    layoutOutputButtons->addWidget(addButton);
    layoutOutputButtons->addWidget(deleteButton);
    layoutGroupBoxes->addLayout(layoutOutputButtons);
    layoutGroupBoxes->addWidget(groupInfo);
    layoutGroupBoxes->setStretchFactor(layoutOutputButtons,0);
    hLayout->addWidget(okButton);
    hLayout->addWidget(cancelButton);
    hLayout->addStretch(100);

    layout->addLayout(layoutLines);
    layout->addLayout(layoutGroupBoxes);
    layout->addLayout(hLayout);

    QObject::connect(comboType, &QComboBox::currentTextChanged, this, [this, groupInfo](const QString& text) {changeGroupBoxInfo(groupInfo);});
    QObject::connect(addButton, &QPushButton::clicked, this, [this, listOutputs]() {addOutput(listOutputs);});
    QObject::connect(deleteButton, &QPushButton::clicked, this, [this, listOutputs]() {deleteOutput(listOutputs);});
    QObject::connect(okButton, &QPushButton::clicked, &dialog, &QDialog::accept);
    QObject::connect(cancelButton, &QPushButton::clicked, &dialog, &QDialog::reject);

    if (dialog.exec() == QDialog::Accepted) {
        QString id = lineID->text();
        if(devicesMap.contains(id)){
            QMessageBox::warning(this, "Ошибка!", "Такое устройство уже есть!");
            return;
        }
        QListWidgetItem *item = new QListWidgetItem();
        item->setText(id);
        QString model = lineModel->text();
        QString type = comboType->currentText();
        QList<QString> outputs;
        QList<QString> info;
        QGridLayout *groupInfoLayout = qobject_cast<QGridLayout*>(groupInfo->layout());
        if (!groupInfoLayout->isEmpty()){
            for(int i = 0; i<groupInfoLayout->rowCount(); i++){
                QWidget *layoutItem = groupInfoLayout->itemAtPosition(i, 1)->widget();
                if (qobject_cast<QLineEdit*>(layoutItem)){
                    QLineEdit *lineEdit = qobject_cast<QLineEdit*>(layoutItem);
                    info << lineEdit->text();
                }
                else if (qobject_cast<QDoubleSpinBox*>(layoutItem)){
                    QDoubleSpinBox *spin = qobject_cast<QDoubleSpinBox*>(layoutItem);
                    info << QString::number(spin->value(), 'f');
                }
                else if (qobject_cast<QSpinBox*>(layoutItem)){
                    QSpinBox *spin = qobject_cast<QSpinBox*>(layoutItem);
                    info << QString::number(spin->value());
                }

            }
        }
        for(int i = 0; i < listOutputs->count(); i++) {
            outputs << listOutputs->item(i)->text();
        }
        ExperimentDeviceInfo deviceInfo;
        deviceInfo.id = id;
        deviceInfo.model = model;
        deviceInfo.type = type;
        deviceInfo.outputs = outputs;
        if (type == "IMU"){
            deviceInfo.imuInfo.instabBias = info.at(0).toDouble();
            deviceInfo.imuInfo.randomWalk = info.at(1).toDouble();
            deviceInfo.imuInfo.initialError = info.at(2).toDouble();
        }
        else if (type == "Камера"){
            deviceInfo.cameraInfo.fps = info.at(0).toInt();
            deviceInfo.cameraInfo.heigt = info.at(1).toInt();
            deviceInfo.cameraInfo.width = info.at(2).toInt();
        }
        else if (type == "Антенна"){
            deviceInfo.antennaInfo.confFileDirectory = info.at(0);
        }
        devicesMap[id] = deviceInfo;
        ui->listWidgetDevices->addItem(item);
    }
}
/// TOASK: В тз не написанно про нередактируемость полей этого окна, поэтому я ничего не делал нередактируемым
void experimentConfigurationDialog::editDevice()
{
    if (ui->listWidgetDevices->selectedItems().isEmpty()) return;

    QListWidgetItem *item = ui->listWidgetDevices->currentItem();
    QString id = item->text();
    ExperimentDeviceInfo data = devicesMap[id];
    QString model = data.model;
    QString type = data.type;
    QList<QString> outputs = data.outputs;
    QList<QString> info;
    if (type == "IMU"){
        info << QString::number(data.imuInfo.instabBias,'f') << QString::number(data.imuInfo.randomWalk,'f') << QString::number(data.imuInfo.initialError,'f');
    }
    else if (type == "Камера"){
        info << QString::number(data.cameraInfo.fps) << QString::number(data.cameraInfo.heigt) << QString::number(data.cameraInfo.width);
    }
    else if (type == "Антенна"){
        info << data.antennaInfo.confFileDirectory;
    }

    QDialog dialog(this);
    dialog.setWindowTitle("Редактор устройств");
    QVBoxLayout *layout = new QVBoxLayout(&dialog);
    QHBoxLayout *layoutLines = new QHBoxLayout();
    QHBoxLayout *layoutGroupBoxes = new QHBoxLayout();
    QHBoxLayout *hLayout = new QHBoxLayout();
    QLabel *labelID = new QLabel("ID: ");
    QLineEdit *lineID = new QLineEdit(id);
    QLabel *labelModel = new QLabel("Модель: ");
    QLineEdit *lineModel = new QLineEdit(model);
    QLabel *labelType = new QLabel("Тип: ");
    QComboBox *comboType = new QComboBox();
    comboType->addItems({"Приёмник", "IMU", "Камера", "Одометр", "Антенна", "Сплиттер", "Другое"});
    QGroupBox *groupOutputs = new QGroupBox("Выводы");
    groupOutputs->setAlignment(Qt::AlignHCenter);
    QGroupBox *groupInfo = new QGroupBox("Дополнительная информация");
    groupInfo->setAlignment(Qt::AlignHCenter);
    QGridLayout *groupInfoLayoutOrigin = new QGridLayout();
    groupInfo->setLayout(groupInfoLayoutOrigin);
    QVBoxLayout *groupOutputsLayout = new QVBoxLayout();
    QListWidget *listOutputs = new QListWidget();
    listOutputs->addItems(outputs);
    for(int i = 0; i < listOutputs->count(); i++) {
        listOutputs->item(i)->setFlags(Qt::ItemIsEnabled | Qt::ItemIsSelectable | Qt::ItemIsEditable);
    }
    QVBoxLayout *layoutOutputButtons = new QVBoxLayout();
    QPushButton *addButton = new QPushButton("");
    addButton->setStyleSheet("image: url(:/resources/add.png);");
    QPushButton *deleteButton = new QPushButton("");
    deleteButton->setStyleSheet("image: url(:/resources/delete.png);");

    QPushButton *okButton = new QPushButton("Готово");
    QPushButton *cancelButton = new QPushButton("Отмена");

    layoutLines->addWidget(labelID);
    layoutLines->addWidget(lineID);
    layoutLines->addWidget(labelModel);
    layoutLines->addWidget(lineModel);
    layoutLines->addWidget(labelType);
    layoutLines->addWidget(comboType);

    groupOutputs->setLayout(groupOutputsLayout);
    groupOutputs->layout()->addWidget(listOutputs);
    layoutGroupBoxes->addWidget(groupOutputs);
    layoutOutputButtons->addWidget(addButton);
    layoutOutputButtons->addWidget(deleteButton);
    layoutGroupBoxes->addLayout(layoutOutputButtons);
    layoutGroupBoxes->addWidget(groupInfo);
    layoutGroupBoxes->setStretchFactor(layoutOutputButtons,0);
    hLayout->addWidget(okButton);
    hLayout->addWidget(cancelButton);
    hLayout->addStretch(100);

    layout->addLayout(layoutLines);
    layout->addLayout(layoutGroupBoxes);
    layout->addLayout(hLayout);

    QObject::connect(comboType, &QComboBox::currentTextChanged, this, [this, groupInfo](const QString& text) {changeGroupBoxInfo(groupInfo);});
    QObject::connect(addButton, &QPushButton::clicked, this, [this, listOutputs]() {addOutput(listOutputs);});
    QObject::connect(deleteButton, &QPushButton::clicked, this, [this, listOutputs]() {deleteOutput(listOutputs);});
    QObject::connect(okButton, &QPushButton::clicked, &dialog, &QDialog::accept);
    QObject::connect(cancelButton, &QPushButton::clicked, &dialog, &QDialog::reject);

    comboType->setCurrentText(type);
    QGridLayout *groupInfoLayout = qobject_cast<QGridLayout*>(groupInfo->layout());
    if (!groupInfoLayout->isEmpty()){
        for(int i = 0; i<groupInfoLayout->rowCount(); i++){
            QWidget *layoutItem = groupInfoLayout->itemAtPosition(i, 1)->widget();
            if (qobject_cast<QLineEdit*>(layoutItem)){
                QLineEdit *lineEdit = qobject_cast<QLineEdit*>(layoutItem);
                lineEdit->setText(info.at(i));
            }
            else if (qobject_cast<QDoubleSpinBox*>(layoutItem)){
                QDoubleSpinBox *spin = qobject_cast<QDoubleSpinBox*>(layoutItem);
                spin->setValue(info.at(i).toDouble());
            }
            else if (qobject_cast<QSpinBox*>(layoutItem)){
                QSpinBox *spin = qobject_cast<QSpinBox*>(layoutItem);
                spin->setValue(info.at(i).toInt());
            }
        }
    }
    if (dialog.exec() == QDialog::Accepted) {
        QGridLayout *groupInfoLayout = qobject_cast<QGridLayout*>(groupInfo->layout());
        QListWidgetItem *item = new QListWidgetItem();
        QString id = lineID->text();
        item->setText(id);
        ExperimentDeviceInfo deviceInfo;
        QString model = lineModel->text();
        QString type = comboType->currentText();
        QList<QString> outputs;
        QList<QString> info;
        if (!groupInfoLayout->isEmpty()){
            for(int i = 0; i<groupInfoLayout->rowCount(); i++){
                QWidget *layoutItem = groupInfoLayout->itemAtPosition(i, 1)->widget();
                if (qobject_cast<QLineEdit*>(layoutItem)){
                    QLineEdit *lineEdit = qobject_cast<QLineEdit*>(layoutItem);
                    info << lineEdit->text();
                }
                else if (qobject_cast<QDoubleSpinBox*>(layoutItem)){
                    QDoubleSpinBox *spin = qobject_cast<QDoubleSpinBox*>(layoutItem);
                    info << QString::number(spin->value(), 'f');
                }
                else if (qobject_cast<QSpinBox*>(layoutItem)){
                    QSpinBox *spin = qobject_cast<QSpinBox*>(layoutItem);
                    info << QString::number(spin->value());
                }

            }
        }
        for(int i = 0; i < listOutputs->count(); i++) {
            outputs << listOutputs->item(i)->text();
        }
        deviceInfo.id = id;
        deviceInfo.model = model;
        deviceInfo.type = type;
        deviceInfo.outputs = outputs;
        if (type == "IMU"){
            deviceInfo.imuInfo.instabBias = info.at(0).toDouble();
            deviceInfo.imuInfo.randomWalk = info.at(1).toDouble();
            deviceInfo.imuInfo.initialError = info.at(2).toDouble();
        }
        else if (type == "Камера"){
            deviceInfo.cameraInfo.fps = info.at(0).toInt();
            deviceInfo.cameraInfo.heigt = info.at(1).toInt();
            deviceInfo.cameraInfo.width = info.at(2).toInt();
        }
        else if (type == "Антенна"){
            deviceInfo.antennaInfo.confFileDirectory = info.at(0);
        }
        devicesMap.remove(id);
        devicesMap[id] = deviceInfo;
        int row = ui->listWidgetDevices->currentRow();
        delete ui->listWidgetDevices->takeItem(row);
        ui->listWidgetDevices->insertItem(row, item);
    }
}

void experimentConfigurationDialog::sortListWidgetByDeviceType(QListWidget* listWidget, Qt::SortOrder order)
{
    if (!listWidget || listWidget->count() <= 1) return;

    // Используем QList для пар вместо QMap
    QList<QPair<ExperimentDeviceInfo, QListWidgetItem*>> items;

    while (listWidget->count() > 0) {
        QListWidgetItem* item = listWidget->takeItem(0);
        ExperimentDeviceInfo info = devicesMap[item->text()];
        items.append(qMakePair(info, item));
    }

    std::sort(items.begin(), items.end(),
              [order](const QPair<ExperimentDeviceInfo, QListWidgetItem*>& a,
                      const QPair<ExperimentDeviceInfo, QListWidgetItem*>& b) {
                  const ExperimentDeviceInfo& infoA = a.first;
                  const ExperimentDeviceInfo& infoB = b.first;

                  QString typeA = infoA.type;
                  QString typeB = infoB.type;

                  if (typeA == typeB) {
                      QString idA = infoA.id;
                      QString idB = infoB.id;
                      bool less = idA < idB;
                      return order == Qt::AscendingOrder ? less : !less;
                  } else {
                      bool less = typeA < typeB;
                      return order == Qt::AscendingOrder ? less : !less;
                  }
              });

    for (const auto& pair : items) {
        listWidget->addItem(pair.second);
    }
}

void experimentConfigurationDialog::sortDevices()
{
    sortListWidgetByDeviceType(ui->listWidgetDevices, devicesSort);
    if (devicesSort == Qt::AscendingOrder){
        devicesSort = Qt::DescendingOrder;
    }
    else{
        devicesSort = Qt::AscendingOrder;
    }

}

void experimentConfigurationDialog::deleteDevice()
{
    if (!ui->listWidgetDevices->selectedItems().isEmpty()){
        QListWidgetItem *item = ui->listWidgetDevices->takeItem(ui->listWidgetDevices->currentRow());
        devicesMap.remove(item->text());
        delete item;
    }
}

void experimentConfigurationDialog::changeGroupBoxSettings(QGroupBox *groupSettings)
{
    QComboBox *comboInterface = qobject_cast<QComboBox*>(sender());
    QGridLayout *groupLayoutOld = qobject_cast<QGridLayout*>(groupSettings->layout());
    foreach (auto object, groupSettings->children()) {
        if(!qobject_cast<QGridLayout*>(object)){
            QWidget *widget = qobject_cast<QWidget*>(object);
            groupLayoutOld->removeWidget(widget);
            delete widget;
        }
    }
    delete groupLayoutOld;
    QGridLayout *groupLayoutNew = new QGridLayout();
    groupSettings->setLayout(groupLayoutNew);
    QString interface = comboInterface->currentText();
    if (interface == "Serial"){
        QLabel *labelBaudrate = new QLabel("Скорость передачи данных: ");
        QComboBox *comboBaudrate = new QComboBox();
        QStringList baudrates = {"9600", "19200", "38400", "57600", "115200", "230400", "460800"};
        comboBaudrate->addItems(baudrates);
        QLabel *labelParity = new QLabel("Бит чётности: ");
        QComboBox *comboParity = new QComboBox();
        QStringList parities = {"Нет", "Четный", "Нечетный", "Всегда 1", "Всегда 0"};
        comboParity->addItems(parities);
        QLabel *labelDataBits = new QLabel("Количество бит данных: ");
        QComboBox *comboDataBits = new QComboBox();
        QStringList dataBits = {"5", "6", "7", "8"};
        comboDataBits->addItems(dataBits);
        QLabel *labelStopBits = new QLabel("Количество стоповых битов: ");
        QComboBox *comboStopBits = new QComboBox();
        QStringList stopBits = {"1", "2"};
        comboStopBits->addItems(stopBits);

        groupLayoutNew->addWidget(labelBaudrate, 0, 0);
        groupLayoutNew->addWidget(comboBaudrate, 0, 1);
        groupLayoutNew->addWidget(labelParity, 1, 0);
        groupLayoutNew->addWidget(comboParity, 1, 1);
        groupLayoutNew->addWidget(labelDataBits, 2, 0);
        groupLayoutNew->addWidget(comboDataBits, 2, 1);
        groupLayoutNew->addWidget(labelStopBits, 3, 0);
        groupLayoutNew->addWidget(comboStopBits, 3, 1);
    }
    else if (interface == "CAN"){
        QLabel *labelBaudrate = new QLabel("Скорость передачи данных (Кб/с): ");
        QComboBox *comboBaudrate = new QComboBox();
        comboBaudrate->setEditable(true);
        QStringList baudrates = {"250", "500", "800", "1000"};
        comboBaudrate->addItems(baudrates);

        groupLayoutNew->addWidget(labelBaudrate, 0, 0);
        groupLayoutNew->addWidget(comboBaudrate, 0, 1);

    }
    else if (interface == "Коакс. кабель"){
        QLabel *labelLength = new QLabel("Длина кабеля: ");
        QLineEdit *lineLength = new QLineEdit();
        QLabel *labelMaterial = new QLabel("Материал: ");
        QLineEdit *lineMaterial = new QLineEdit();
        QLabel *labelSignalLoss = new QLabel("Величина потери сигнала в дБГц: ");
        QSpinBox *spinSignalLoss = new QSpinBox();

        groupLayoutNew->addWidget(labelLength, 0, 0);
        groupLayoutNew->addWidget(lineLength, 0, 1);
        groupLayoutNew->addWidget(labelMaterial, 1, 0);
        groupLayoutNew->addWidget(lineMaterial, 1, 1);
        groupLayoutNew->addWidget(labelSignalLoss, 2, 0);
        groupLayoutNew->addWidget(spinSignalLoss, 2, 1);
    }
    else if (interface == "TCP"){
        QLabel *labelAdress = new QLabel("Адрес сервера: ");
        QLineEdit *lineAdress = new QLineEdit();
        QLabel *labelPortNumber = new QLabel("Номер порта: ");
        QLineEdit *linePortNumber = new QLineEdit();

        groupLayoutNew->addWidget(labelAdress, 0, 0);
        groupLayoutNew->addWidget(lineAdress, 0, 1);
        groupLayoutNew->addWidget(labelPortNumber, 1, 0);
        groupLayoutNew->addWidget(linePortNumber, 1, 1);
    }
}

void experimentConfigurationDialog::checkConnectionSettingsFields(QComboBox *comboDevice1, QComboBox *comboDevice2, QDialog *dialog)
{
    if (comboDevice1->currentText() == comboDevice2->currentText()){
        QMessageBox::warning(this, "Ошибка", "Нельзя соединять устройство с самим собой!");
        return;
    }
    if (comboDevice1->currentText().isEmpty() || comboDevice2->currentText().isEmpty()){
        QMessageBox::warning(this, "Ошибка", "Устройства не выбраны!");
        return;
    }
    else dialog->accept();
}

void experimentConfigurationDialog::changeDeviceOutputs(QComboBox *comboDeviceOutput){
    comboDeviceOutput->clear();
    QComboBox* comboDevice = qobject_cast<QComboBox*>(sender());
    QString deviceName = comboDevice->currentText();
    ExperimentDeviceInfo data = devicesMap[deviceName];
    QStringList outputs = data.outputs;
    comboDeviceOutput->addItems(outputs);
}

void experimentConfigurationDialog::addConnection()
{
    QDialog dialog(this);
    dialog.setWindowTitle("Редактор связей");
    QVBoxLayout *layout = new QVBoxLayout(&dialog);
    QHBoxLayout *mainLayout = new QHBoxLayout();
    QGridLayout *comboLayout = new QGridLayout();
    QGroupBox *groupSettings = new QGroupBox("Настройка связи");
    groupSettings->setAlignment(Qt::AlignHCenter);
    QGridLayout *settingsLayout = new QGridLayout();
    groupSettings->setLayout(settingsLayout);
    QLabel *labelDevice1 = new QLabel("Ус-во 1: ");
    QComboBox *comboDevice1 = new QComboBox();
    QLabel *labelDeviceOutput1 = new QLabel("Вывод ус-ва 1: ");
    QComboBox *comboDeviceOutput1 = new QComboBox();
    QLabel *labelDevice2 = new QLabel("Ус-во 2: ");
    QComboBox *comboDevice2 = new QComboBox();
    QLabel *labelDeviceOutput2 = new QLabel("Вывод ус-ва 2: ");
    QComboBox *comboDeviceOutput2 = new QComboBox();
    QLabel *labelInteface = new QLabel("Тип связи: ");
    QComboBox *comboInterface = new QComboBox();

    QStringList devices;
    for (int i = 0; i < ui->listWidgetDevices->count(); ++i) {
        devices.append(ui->listWidgetDevices->item(i)->text());
    }
    QStringList interfaces =  {"Serial", "CAN", "Коакс. кабель", "TCP"};
    comboInterface->addItems(interfaces);

    comboDevice1->addItems(devices);
    comboDevice2->addItems(devices);
    comboDevice1->setCurrentIndex(-1);
    comboDevice2->setCurrentIndex(-1);
    comboInterface->setCurrentIndex(-1);


    comboLayout->addWidget(labelDevice1,0,0);
    comboLayout->addWidget(comboDevice1,0,1);
    comboLayout->addWidget(labelDeviceOutput1,1,0);
    comboLayout->addWidget(comboDeviceOutput1,1,1);
    comboLayout->addWidget(labelDevice2,2,0);
    comboLayout->addWidget(comboDevice2,2,1);
    comboLayout->addWidget(labelDeviceOutput2,3,0);
    comboLayout->addWidget(comboDeviceOutput2,3,1);
    comboLayout->addWidget(labelInteface,4,0);
    comboLayout->addWidget(comboInterface,4,1);


    QHBoxLayout *hLayout = new QHBoxLayout();
    QPushButton *okButton = new QPushButton("Готово");
    QPushButton *cancelButton = new QPushButton("Отмена");


    hLayout->addWidget(okButton);
    hLayout->addWidget(cancelButton);
    hLayout->addStretch(100);

    mainLayout->addLayout(comboLayout);
    mainLayout->addWidget(groupSettings);
    layout->addLayout(mainLayout);
    layout->addLayout(hLayout);


    QObject::connect(comboInterface, &QComboBox::currentTextChanged, this, [this, groupSettings]() {changeGroupBoxSettings(groupSettings);});
    QObject::connect(comboDevice1, &QComboBox::currentTextChanged, this, [this, comboDeviceOutput1]() {changeDeviceOutputs(comboDeviceOutput1);});
    QObject::connect(comboDevice2, &QComboBox::currentTextChanged, this, [this, comboDeviceOutput2]() {changeDeviceOutputs(comboDeviceOutput2);});
    QObject::connect(okButton, &QPushButton::clicked, this, [this, comboDevice1, comboDevice2, &dialog]() {checkConnectionSettingsFields(comboDevice1, comboDevice2, &dialog);});
    QObject::connect(cancelButton, &QPushButton::clicked, &dialog, &QDialog::reject);

    if (dialog.exec() == QDialog::Accepted) {
        QString device1 = comboDevice1->currentText();
        QString device2 = comboDevice2->currentText();
        QString port1 = comboDeviceOutput1->currentText();
        QString port2 = comboDeviceOutput2->currentText();
        QString connectionType = comboInterface->currentText();
        QTableWidgetItem* device1Item = new QTableWidgetItem(device1);
        QTableWidgetItem* device2Item = new QTableWidgetItem(device2);
        QTableWidgetItem* device1OutputItem = new QTableWidgetItem(port1);
        QTableWidgetItem* device2OutputItem = new QTableWidgetItem(port2);
        QTableWidgetItem* connectionTypeItem = new QTableWidgetItem(connectionType);
        ExperimentConnectionInfo info;
        info.device1 = device1;
        info.device2 = device2;
        info.port1 = port1;
        info.port2 = port2;
        info.connectionType = connectionType;
        int index = 0;
        foreach (auto obj, groupSettings->children()) {
            if (!(qobject_cast<QLineEdit*>(obj) || qobject_cast<QComboBox*>(obj) || qobject_cast<QSpinBox*>(obj))) continue;
            QString string;
            if (qobject_cast<QLineEdit*>(obj)){
                QLineEdit* line = qobject_cast<QLineEdit*>(obj);
                string = line->text();
            }
            else if (qobject_cast<QComboBox*>(obj)){
                QComboBox* combo = qobject_cast<QComboBox*>(obj);
                string = combo->currentText();
            }
            else if (qobject_cast<QSpinBox*>(obj)){
                QSpinBox* spin = qobject_cast<QSpinBox*>(obj);
                string = QString::number(spin->value());
            }
            if (connectionType == "Serial"){
                switch (index) {
                case 0:
                    info.serialInfo.baudrate = string.toInt();
                    break;
                case 1:
                    info.serialInfo.parity = string;
                    break;
                case 2:
                    info.serialInfo.dataBits = string.toInt();
                    break;
                case 3:
                    info.serialInfo.stopBits = string.toInt();
                    break;
                default:
                    break;
                }
            }
            else if (connectionType == "CAN"){
                switch (index) {
                case 0:
                    info.canInfo.baudrate = string.toInt();
                    break;
                default:
                    break;
                }
            }
            else if (connectionType == "Коакс. кабель"){
                switch (index) {
                case 0:
                    info.coaxCableInfo.length = string;
                    break;
                case 1:
                    info.coaxCableInfo.material = string;
                    break;
                case 2:
                    info.coaxCableInfo.dataLoss = string.toInt();
                    break;
                default:
                    break;
                }
            }
            else if (connectionType == "TCP"){
                switch (index) {
                case 0:
                    info.tcpInfo.adress = string;
                    break;
                case 1:
                    info.tcpInfo.port = string;
                    break;
                default:
                    break;
                }
            }
            index++;
        }
        QString key = device1+'&'+device2;
        connectionsMap[key] = info;
        qDebug() << connectionsMap[key].coaxCableInfo.length << connectionsMap[key].coaxCableInfo.material << connectionsMap[key].coaxCableInfo.dataLoss;
        int row = ui->tableWidgetConnections->rowCount();
        ui->tableWidgetConnections->setRowCount(row + 1);
        ui->tableWidgetConnections->setItem(row, 0, device1Item);
        ui->tableWidgetConnections->setItem(row, 1, device1OutputItem);
        ui->tableWidgetConnections->setItem(row, 2, connectionTypeItem);
        ui->tableWidgetConnections->setItem(row, 3, device2Item);
        ui->tableWidgetConnections->setItem(row, 4, device2OutputItem);
        setupTableSize(ui->tableWidgetConnections);
    }
}

void experimentConfigurationDialog::editConnection()
{
    if (ui->tableWidgetConnections->selectedItems().isEmpty()){
        QMessageBox::warning(this, "Ошибка", "Устройство не выбрано!");
        return;
    }
    int row = ui->tableWidgetConnections->currentRow();

    QDialog dialog(this);
    dialog.setWindowTitle("Редактор связей");
    QVBoxLayout *layout = new QVBoxLayout(&dialog);
    QHBoxLayout *mainLayout = new QHBoxLayout();
    QGridLayout *comboLayout = new QGridLayout();
    QGroupBox *groupSettings = new QGroupBox("Настройка связи");
    groupSettings->setAlignment(Qt::AlignHCenter);
    QGridLayout *settingsLayout = new QGridLayout();
    groupSettings->setLayout(settingsLayout);
    QLabel *labelDevice1 = new QLabel("Ус-во 1: ");
    QComboBox *comboDevice1 = new QComboBox();
    QLabel *labelDeviceOutput1 = new QLabel("Вывод ус-ва 1: ");
    QComboBox *comboDeviceOutput1 = new QComboBox();
    QLabel *labelDevice2 = new QLabel("Ус-во 2: ");
    QComboBox *comboDevice2 = new QComboBox();
    QLabel *labelDeviceOutput2 = new QLabel("Вывод ус-ва 2: ");
    QComboBox *comboDeviceOutput2 = new QComboBox();
    QLabel *labelInteface = new QLabel("Тип связи: ");
    QComboBox *comboInterface = new QComboBox();

    QStringList devices;

    QHBoxLayout *hLayout = new QHBoxLayout();
    QPushButton *okButton = new QPushButton("Готово");
    QPushButton *cancelButton = new QPushButton("Отмена");

    QObject::connect(comboInterface, &QComboBox::currentTextChanged, this, [this, groupSettings]() {changeGroupBoxSettings(groupSettings);});
    QObject::connect(comboDevice1, &QComboBox::currentTextChanged, this, [this, comboDeviceOutput1]() {changeDeviceOutputs(comboDeviceOutput1);});
    QObject::connect(comboDevice2, &QComboBox::currentTextChanged, this, [this, comboDeviceOutput2]() {changeDeviceOutputs(comboDeviceOutput2);});
    QObject::connect(okButton, &QPushButton::clicked, this, [this, comboDevice1, comboDevice2, &dialog]() {checkConnectionSettingsFields(comboDevice1, comboDevice2, &dialog);});
    QObject::connect(cancelButton, &QPushButton::clicked, &dialog, &QDialog::reject);

    for (int i = 0; i < ui->listWidgetDevices->count(); ++i) {
        devices.append(ui->listWidgetDevices->item(i)->text());
    }
    comboDevice1->addItems(devices);
    comboDevice2->addItems(devices);
    QStringList interfaces =  {"Serial", "CAN", "Коакс. кабель", "TCP"};
    comboInterface->addItems(interfaces);
    QString device1 = ui->tableWidgetConnections->item(row,0)->text();
    QString device2 = ui->tableWidgetConnections->item(row,3)->text();
    QString port1 = ui->tableWidgetConnections->item(row,1)->text();
    QString port2 = ui->tableWidgetConnections->item(row,4)->text();
    QString connectionType = ui->tableWidgetConnections->item(row,2)->text();
    comboDevice1->setCurrentText(device1);
    comboDevice2->setCurrentText(device2);
    comboDeviceOutput1->setCurrentText(port1);
    comboDeviceOutput2->setCurrentText(port2);
    comboInterface->setCurrentText(connectionType);

    ExperimentConnectionInfo data = connectionsMap[device1+'&'+device2];
    QStringList info;
    if (connectionType == "Serial"){
        info << QString::number(data.serialInfo.baudrate) << data.serialInfo.parity << QString::number(data.serialInfo.dataBits) << QString::number(data.serialInfo.stopBits);
    }
    else if (connectionType == "CAN"){
        info << QString::number(data.canInfo.baudrate);
    }
    else if (connectionType == "Коакс. кабель"){
        info << data.coaxCableInfo.length << data.coaxCableInfo.material << QString::number(data.coaxCableInfo.dataLoss);
    }
    else if (connectionType == "TCP"){
        info << data.tcpInfo.adress << data.tcpInfo.port;
    }
    qDebug() << info;
    int index = 0;
    foreach (auto obj, groupSettings->children()) {
        if (!(qobject_cast<QLineEdit*>(obj) || qobject_cast<QComboBox*>(obj) || qobject_cast<QSpinBox*>(obj))) continue;
        if (qobject_cast<QLineEdit*>(obj)){
            QLineEdit* line = qobject_cast<QLineEdit*>(obj);
            line->setText(info.at(index));
        }
        else if (qobject_cast<QComboBox*>(obj)){
            QComboBox* combo = qobject_cast<QComboBox*>(obj);
            combo->setCurrentText(info.at(index));
        }
        else if (qobject_cast<QSpinBox*>(obj)){
            QSpinBox* spin = qobject_cast<QSpinBox*>(obj);
            spin->setValue(info.at(index).toInt());
        }
        index++;
    }

    comboLayout->addWidget(labelDevice1,0,0);
    comboLayout->addWidget(comboDevice1,0,1);
    comboLayout->addWidget(labelDeviceOutput1,1,0);
    comboLayout->addWidget(comboDeviceOutput1,1,1);
    comboLayout->addWidget(labelDevice2,2,0);
    comboLayout->addWidget(comboDevice2,2,1);
    comboLayout->addWidget(labelDeviceOutput2,3,0);
    comboLayout->addWidget(comboDeviceOutput2,3,1);
    comboLayout->addWidget(labelInteface,4,0);
    comboLayout->addWidget(comboInterface,4,1);


    hLayout->addWidget(okButton);
    hLayout->addWidget(cancelButton);
    hLayout->addStretch(100);

    mainLayout->addLayout(comboLayout);
    mainLayout->addWidget(groupSettings);
    layout->addLayout(mainLayout);
    layout->addLayout(hLayout);


    if (dialog.exec() == QDialog::Accepted) {
        QString device1 = comboDevice1->currentText();
        QString device2 = comboDevice2->currentText();
        QString port1 = comboDeviceOutput1->currentText();
        QString port2 = comboDeviceOutput2->currentText();
        QString connectionType = comboInterface->currentText();
        QTableWidgetItem* device1Item = new QTableWidgetItem(device1);
        QTableWidgetItem* device2Item = new QTableWidgetItem(device2);
        QTableWidgetItem* device1OutputItem = new QTableWidgetItem(port1);
        QTableWidgetItem* device2OutputItem = new QTableWidgetItem(port2);
        QTableWidgetItem* connectionTypeItem = new QTableWidgetItem(connectionType);
        ExperimentConnectionInfo info;
        info.device1 = device1;
        info.device2 = device2;
        info.port1 = port1;
        info.port2 = port2;
        info.connectionType = connectionType;
        int index = 0;
        foreach (auto obj, groupSettings->children()) {
            if (!(qobject_cast<QLineEdit*>(obj) || qobject_cast<QComboBox*>(obj) || qobject_cast<QSpinBox*>(obj))) continue;
            QString string;
            if (qobject_cast<QLineEdit*>(obj)){
                QLineEdit* line = qobject_cast<QLineEdit*>(obj);
                string = line->text();
            }
            else if (qobject_cast<QComboBox*>(obj)){
                QComboBox* combo = qobject_cast<QComboBox*>(obj);
                string = combo->currentText();
            }
            else if (qobject_cast<QSpinBox*>(obj)){
                QSpinBox* spin = qobject_cast<QSpinBox*>(obj);
                string = QString::number(spin->value());
            }
            if (connectionType == "Serial"){
                switch (index) {
                case 0:
                    info.serialInfo.baudrate = string.toInt();
                    break;
                case 1:
                    info.serialInfo.parity = string;
                    break;
                case 2:
                    info.serialInfo.dataBits = string.toInt();
                    break;
                case 3:
                    info.serialInfo.stopBits = string.toInt();
                    break;
                default:
                    break;
                }
            }
            else if (connectionType == "CAN"){
                switch (index) {
                case 0:
                    info.canInfo.baudrate = string.toInt();
                    break;
                default:
                    break;
                }
            }
            else if (connectionType == "Коакс. кабель"){
                switch (index) {
                case 0:
                    info.coaxCableInfo.length = string;
                    break;
                case 1:
                    info.coaxCableInfo.material = string;
                    break;
                case 2:
                    info.coaxCableInfo.dataLoss = string.toInt();
                    break;
                default:
                    break;
                }
            }
            else if (connectionType == "TCP"){
                switch (index) {
                case 0:
                    info.tcpInfo.adress = string;
                    break;
                case 1:
                    info.tcpInfo.port = string;
                    break;
                default:
                    break;
                }
            }
            index++;
        }
        QString key = device1+'&'+device2;
        connectionsMap.remove(key);
        connectionsMap[key] = info;
        ui->tableWidgetConnections->removeRow(row);
        ui->tableWidgetConnections->insertRow(row);
        ui->tableWidgetConnections->setItem(row, 0, device1Item);
        ui->tableWidgetConnections->setItem(row, 1, device1OutputItem);
        ui->tableWidgetConnections->setItem(row, 2, connectionTypeItem);
        ui->tableWidgetConnections->setItem(row, 3, device2Item);
        ui->tableWidgetConnections->setItem(row, 4, device2OutputItem);
        setupTableSize(ui->tableWidgetConnections);
    }
}

void experimentConfigurationDialog::deleteConnection()
{
    if (ui->tableWidgetConnections->selectedItems().isEmpty()){
        QMessageBox::warning(this, "Ошибка", "Устройство не выбрано!");
        return;
    }
    int row = ui->tableWidgetConnections->currentRow();
    connectionsMap.remove(ui->tableWidgetConnections->item(row, 0)->text() + '&' + ui->tableWidgetConnections->item(row, 3)->text());
    ui->tableWidgetConnections->removeRow(ui->tableWidgetConnections->currentRow());
}

// 2 я вкладка

// 3я вклада

/// TODO: функции
/// addParametersEvent - слот, дающий выбор устройства перед добавлением groupBox
void experimentConfigurationDialog::addParametersEvent(){
    QStringList keys = devicesMap.keys();
    if (keys.isEmpty()) return;
    QStringList availableDevices;
    foreach (QString key, keys) {
        if (!groupBoxParametersMap.contains(key)) availableDevices.append(key);
    }
    if (availableDevices.isEmpty()) return;

    QDialog *dialog = new QDialog(this);
    QVBoxLayout *layout = new QVBoxLayout(dialog);
    QComboBox *combo = new QComboBox(dialog);
    QPushButton *okButton = new QPushButton("OK");
    combo->addItems(availableDevices);

    layout->addWidget(combo);
    layout->addWidget(okButton);

    QObject::connect(okButton, &QPushButton::clicked, dialog, &QDialog::accept);

    if (dialog->exec() == QDialog::Accepted) {
        QString id = combo->currentText();
        QGroupBox* group = createParametersGroup(id);
        int index = ui->areaDeviceParameters->addGroup(group);
        ExperimentGroupBoxParametersInfo info;
        info.deviceName = id;
        info.column = index%2;
        info.row = index/2;
        info.group = group;
        bool positionCheck = false;
        bool boresightCheck = false;
        bool leverarmCheck = false;
        QString type = devicesMap[id].type;
        if (type == "Приёмник"){
            positionCheck = true;
        }
        else if (type == "IMU" || type == "Камера"){
            positionCheck = true;
            boresightCheck = true;
        }
        else if (type == "Одометр"){
            leverarmCheck = true;
        }
        info.positionCheck  = positionCheck;
        info.boresightCheck = boresightCheck;
        info.leverarmCheck  = leverarmCheck;
        groupBoxParametersMap[id] = info;
    }
}

QGroupBox* experimentConfigurationDialog::createParameterGroup(QString parameter, QWidget* parent){
    QGroupBox* parameterGroup = new QGroupBox(parent);
    QGridLayout* parameterLayout = new QGridLayout(parameterGroup);
    QLabel* labelX = new QLabel("X", parameterGroup);
    QLabel* labelY = new QLabel("Y", parameterGroup);
    QLabel* labelZ = new QLabel("Z", parameterGroup);
    QSpinBox* spinX = new QSpinBox(parameterGroup);
    QSpinBox* spinY = new QSpinBox(parameterGroup);
    QSpinBox* spinZ = new QSpinBox(parameterGroup);
    QPushButton* buttonRemove = new QPushButton(parameterGroup);

    spinX->setMaximum(10e6); spinX->setMinimum(-10e6);
    spinY->setMaximum(10e6); spinY->setMinimum(-10e6);
    spinZ->setMaximum(10e6); spinZ->setMinimum(-10e6);
    buttonRemove->setStyleSheet("image: url(:/resources/remove.png);");
    connect(buttonRemove, SIGNAL(clicked(bool)), this, SLOT(removeParameter()));
    parameterGroup->setTitle(parameter);
    parameterLayout->addWidget(labelX,0,0);
    parameterLayout->addWidget(spinX,0,1);
    parameterLayout->addWidget(labelY,1,0);
    parameterLayout->addWidget(spinY,1,1);
    parameterLayout->addWidget(labelZ,2,0);
    parameterLayout->addWidget(spinZ,2,1);
    parameterLayout->addWidget(buttonRemove,0,2,3,1);
    parameterLayout->setColumnStretch(1,1);
    parameterGroup->setLayout(parameterLayout);
    return parameterGroup;
}
/// createParametersGroup - конструктор groupBox'а
QGroupBox* experimentConfigurationDialog::createParametersGroup(QString id, bool isCheckType){
    QGroupBox* group = new QGroupBox(ui->areaDeviceParameters);
    group->setTitle(id);
    QVBoxLayout* layout = new QVBoxLayout(group);
    QVBoxLayout* parametersLayout = new QVBoxLayout(group);
    QHBoxLayout* buttonsLayout = new QHBoxLayout(group);
    QPushButton* buttonAdd = new QPushButton(group);
    connect(buttonAdd, SIGNAL(clicked(bool)), this, SLOT(addParameter()));
    buttonAdd->setStyleSheet("image: url(:/resources/add.png);");
    QPushButton* buttonDelete = new QPushButton(group);
    connect(buttonDelete, SIGNAL(clicked(bool)), this, SLOT(removePrametersGroup()));
    buttonDelete->setStyleSheet("image: url(:/resources/delete.png);");
    if (isCheckType){
        QString type = devicesMap[id].type;
        bool positionCheck = false;
        bool boresightCheck = false;
        bool leverarmCheck = false;
        if (type == "Приёмник"){
            positionCheck = true;
        }
        else if (type == "IMU" || type == "Камера"){
            positionCheck = true;
            boresightCheck = true;
        }
        else if (type == "Одометр"){
            leverarmCheck = true;
        }
        QList<bool> checks = {positionCheck, boresightCheck, leverarmCheck};
        int index = 0;
        foreach (bool check, checks) {
            if (!check) {index++; continue;}
            QString parameter;
            switch (index) {
            case 0:
                parameter = "Позиция";
                break;
            case 1:
                parameter = "Ошибка угла наведения";
                break;
            case 2:
                parameter = "Lever arm";
                break;
            default:
                break;
            }
            QGroupBox* parameterGroup = createParameterGroup(parameter, group);
            parametersLayout->addWidget(parameterGroup);
            index++;
        }
    }
    buttonsLayout->addWidget(buttonAdd);
    QSpacerItem* hSpacer = new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);
    buttonsLayout->addItem(hSpacer);
    buttonsLayout->addWidget(buttonDelete);
    layout->addLayout(parametersLayout);
    QSpacerItem* vSpacer = new QSpacerItem(20, 40, QSizePolicy::Minimum, QSizePolicy::Expanding);
    layout->addItem(vSpacer);
    layout->addLayout(buttonsLayout);
    group->setLayout(layout);
    return group;
}
/// removeParameter - слот для кнопки удаления параметра
void experimentConfigurationDialog::removeParameter(){
    QGroupBox* group = qobject_cast<QGroupBox*>(sender()->parent());
    QGroupBox* groupParent = qobject_cast<QGroupBox*>(group->parent());
    QString parameter = group->title();
    QString id = groupParent->title();
    ExperimentGroupBoxParametersInfo* info = &groupBoxParametersMap[id];
    QList<QPair<bool*, QString>> parametersKeysList = {
        qMakePair(&info->positionCheck, QString("Позиция")),
        qMakePair(&info->boresightCheck, QString("Ошибка угла наведения")),
        qMakePair(&info->leverarmCheck, QString("Lever arm"))
    };
    foreach (auto pair, parametersKeysList) {
        if (pair.second == parameter) {
            *pair.first = false;
        }
    }
    qobject_cast<QVBoxLayout*>(groupParent->layout()->itemAt(0)->layout())->removeWidget(group);
    delete group;
}

QList<QGroupBox*> experimentConfigurationDialog::getParameters(QGroupBox* group){
    QVBoxLayout* layout = qobject_cast<QVBoxLayout*>(group->layout()->itemAt(0)->layout());
    QList<QGroupBox*> groupBoxes;
    for (int i = 0; i < layout->count(); ++i) {
        QGroupBox* group = qobject_cast<QGroupBox*>(layout->itemAt(i)->widget());
        groupBoxes.append(group);
    }
    return groupBoxes;
}

QGroupBox* experimentConfigurationDialog::addParameter(QString parameter, QGroupBox* group)
{
    QGroupBox* parameterGroup = createParameterGroup(parameter, group);
    qobject_cast<QVBoxLayout*>(group->layout()->itemAt(0)->layout())->addWidget(parameterGroup);
    return parameterGroup;
}
/// addParameter - слот для добавления параметра
void experimentConfigurationDialog::addParameter(){
    QGroupBox* group = qobject_cast<QGroupBox*>(sender()->parent());
    QString id = group->title();
    ExperimentGroupBoxParametersInfo* info = &groupBoxParametersMap[id];
    QList<QPair<bool*, QString>> parametersKeysList = {
        qMakePair(&info->positionCheck, QString("Позиция")),
        qMakePair(&info->boresightCheck, QString("Ошибка угла наведения")),
        qMakePair(&info->leverarmCheck, QString("Lever arm"))
    };
    QStringList parameters;
    foreach (auto pair, parametersKeysList) {
        if (!*pair.first) parameters.append(pair.second);
    }
    if (parameters.isEmpty()){
        QMessageBox::warning(this, "Ошибка", "Для этого устройства уже добавленны все параметры!");
        return;
    }
    QDialog *dialog = new QDialog(this);
    QVBoxLayout *layout = new QVBoxLayout(dialog);
    QComboBox *combo = new QComboBox(dialog);
    QPushButton *okButton = new QPushButton("OK");
    combo->addItems(parameters);

    layout->addWidget(combo);
    layout->addWidget(okButton);

    QObject::connect(okButton, &QPushButton::clicked, dialog, &QDialog::accept);

    if (dialog->exec() == QDialog::Accepted) {
        QString parameter = combo->currentText();
        addParameter(parameter, group);
        QList<QPair<bool*, QString>> parametersKeysList = {
            qMakePair(&info->positionCheck, QString("Позиция")),
            qMakePair(&info->boresightCheck, QString("Ошибка угла наведения")),
            qMakePair(&info->leverarmCheck, QString("Lever arm"))
        };
        foreach (auto pair, parametersKeysList) {
            if (pair.second == parameter) {
                *pair.first = true;
            }
        }
    }
}
/// removePrametersGroup - слот для удаления groupBox'а
void experimentConfigurationDialog::removePrametersGroup(){
    QGroupBox* group = qobject_cast<QGroupBox*>(sender()->parent());
    QString id = group->title();
    ExperimentGroupBoxParametersInfo info = groupBoxParametersMap[id];
    ui->areaDeviceParameters->removeGroup(info.row, info.column);
    groupBoxParametersMap.remove(id);

}


// 3я вклада

