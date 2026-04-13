#include "experimentconfigurationdialog.h"
#include "ui_experimentconfigurationdialog.h"

void experimentConfigurationDialog::saveTab()
{
    qDebug() << ui->tabWidget->currentIndex();
}

void experimentConfigurationDialog::addSaveButton(int index)
{
    QPushButton* button = new QPushButton("");
    button->setStyleSheet("image: url(:/resources/save.png);");

    connect(button, &QPushButton::clicked, this, &experimentConfigurationDialog::saveTab);
    ui->tabWidget->tabBar()->setTabButton(index, QTabBar::RightSide, button);
}

experimentConfigurationDialog::experimentConfigurationDialog(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::experimentConfigurationDialog)
{
    ui->setupUi(this);
    foreach (auto object, ui->groupBoxGeneralInfo->children()) {
        if(!qobject_cast<QVBoxLayout*>(object)){
            QWidget *widget = qobject_cast<QWidget*>(object);
            widget->setHidden(true);
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

void experimentConfigurationDialog::openTeamMemberDialog(QString FIO, QString role)
{
    QDialog dialog(this);
    dialog.setWindowTitle("Редактор участника");
    QVBoxLayout *layout = new QVBoxLayout(&dialog);
    QGridLayout *gridLayout = new QGridLayout();
    QLineEdit *lineFIO = new QLineEdit();
    QComboBox *comboRole = new QComboBox();
    QLabel *labelFIO = new QLabel("ФИО");
    QLabel *labelRole = new QLabel("Роль");
    QHBoxLayout *hLayout = new QHBoxLayout();
    QPushButton *okButton = new QPushButton("Готово");
    QPushButton *cancelButton = new QPushButton("Отмена");

    comboRole->addItems({"организатор", "регистратор", "наблюдатель", "водитель"});
    bool isEdit = false;
    if (!FIO.isEmpty()) isEdit = true;
    lineFIO->setText(FIO);
    comboRole->setCurrentText(role);

    gridLayout->addWidget(labelFIO,0,0);
    gridLayout->addWidget(labelRole, 0, 1);
    gridLayout->addWidget(lineFIO, 1, 0);
    gridLayout->addWidget(comboRole, 1, 1);

    hLayout->addStretch(100);
    hLayout->addWidget(okButton);
    hLayout->addWidget(cancelButton);
    hLayout->addStretch(100);

    layout->addLayout(gridLayout);
    layout->addLayout(hLayout);

    QObject::connect(okButton, &QPushButton::clicked, &dialog, &QDialog::accept);
    QObject::connect(cancelButton, &QPushButton::clicked, &dialog, &QDialog::reject);

    if (dialog.exec() == QDialog::Accepted) {
        if (!lineFIO->text().isEmpty()){
            if (isEdit){
                ui->listWidgetTeam->currentItem()->setText(lineFIO->text() + "\t||\t" + comboRole->currentText());
            }
            else {
                QListWidgetItem *item = new QListWidgetItem(lineFIO->text() + "      ||      " + comboRole->currentText());
                ui->listWidgetTeam->addItem(item);
            }
        }
    }
}

void experimentConfigurationDialog::addTeamMember()
{
    openTeamMemberDialog();
}

void experimentConfigurationDialog::editTeamMember()
{
    if (!ui->listWidgetTeam->selectedItems().isEmpty()){
        QString text = ui->listWidgetTeam->currentItem()->text();
        QString FIO = text.split("      ||      ").at(0);
        QString role = text.split("      ||      ").at(1);
        openTeamMemberDialog(FIO, role);
    }
}

void experimentConfigurationDialog::deleteTeamMember()
{
    if (!ui->listWidgetTeam->selectedItems().isEmpty()){
        QListWidgetItem *item = ui->listWidgetTeam->takeItem(ui->listWidgetTeam->currentRow());
        delete item;
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
        ui->labelLocationOfEvent->setHidden(false);
        ui->lineEditLocationOfEvent->setHidden(false);
    }
    else{
        ui->labelAboutVehicle->setHidden(false);
        ui->lineEditAboutVehicle->setHidden(false);
        ui->labelStartPoint->setHidden(false);
        ui->lineEditStartPoint->setHidden(false);
        ui->labelEndPoint->setHidden(false);
        ui->lineEditEndPoint->setHidden(false);
        ui->labelMovementMethod->setHidden(false);
        ui->lineEditMovementMethod->setHidden(false);
    }
}

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
        QLineEdit *lineShiftDistability = new QLineEdit();
        QLabel *labelRandomLuft = new QLabel("Случайное блуждание: ");
        QLineEdit *lineRandomLuft = new QLineEdit();
        QLabel *labelStartError = new QLabel("Начальная погрешность: ");
        QLineEdit *lineStartError = new QLineEdit();

        groupLayoutNew->addWidget(labelShiftDistability, 0, 0);
        groupLayoutNew->addWidget(lineShiftDistability, 0, 1);
        groupLayoutNew->addWidget(labelRandomLuft, 1, 0);
        groupLayoutNew->addWidget(lineRandomLuft, 1, 1);
        groupLayoutNew->addWidget(labelStartError, 2, 0);
        groupLayoutNew->addWidget(lineStartError, 2, 1);
    }
    else if (type == "Камера"){
        QLabel *labelFPS = new QLabel("Кол-во кадров в секунду: ");
        QLineEdit *lineFPS = new QLineEdit();
        QLabel *labelResolution = new QLabel("Разрешение съёмки: ");
        QLineEdit *lineResolution = new QLineEdit();

        groupLayoutNew->addWidget(labelFPS, 0, 0);
        groupLayoutNew->addWidget(lineFPS, 0, 1);
        groupLayoutNew->addWidget(labelResolution, 1, 0);
        groupLayoutNew->addWidget(lineResolution, 1, 1);

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
        if(ui->listWidgetDevices->findItems(lineID->text(),Qt::MatchExactly).isEmpty()){
            QListWidgetItem *item = new QListWidgetItem();
            item->setText(lineID->text());
            QList<QVariant> data;
            QString model = lineModel->text();
            QString type = comboType->currentText();
            QList<QString> outputs;
            QList<QString> info;
            QGridLayout *groupInfoLayout = qobject_cast<QGridLayout*>(groupInfo->layout());
            if (!groupInfoLayout->isEmpty()){
                for(int i = 0; i<groupInfoLayout->rowCount(); i++){
                    QWidget *layoutItem = groupInfoLayout->itemAtPosition(i, 1)->widget();
                    QLineEdit *lineEdit = qobject_cast<QLineEdit*>(layoutItem);
                    if(lineEdit) info << lineEdit->text();
                }
            }
            for(int i = 0; i < listOutputs->count(); i++) {
                outputs << listOutputs->item(i)->text();
            }
            data << model
                 << type
                 << QVariant(outputs)
                 << QVariant(info);
            item->setData(Qt::UserRole, data);
            ui->listWidgetDevices->addItem(item);
        }
        else{
            QMessageBox::warning(this, "Ошибка!", "Такое устройство уже есть!");
            return;
        }
    }
}
/// TOASK: В тз не написанно про нередактируемость полей этого окна, поэтому я ничего не делал нередактируемым
void experimentConfigurationDialog::editDevice()
{
    if (!ui->listWidgetDevices->selectedItems().isEmpty()){
        QListWidgetItem *item = ui->listWidgetDevices->currentItem();
        QList<QVariant> data = item->data(Qt::UserRole).toList();
        QString ID = item->text();
        QString model = data.at(INDEX_DEVICE_MODEL).toString();
        QString type = data.at(INDEX_DEVICE_TYPE).toString();
        QList<QString> outputs = data.at(INDEX_DEVICE_OUTPUTS).toStringList();
        QList<QString> info = data.at(INDEX_DEVICE_INFO).toStringList();

        QDialog dialog(this);
        dialog.setWindowTitle("Редактор устройств");
        QVBoxLayout *layout = new QVBoxLayout(&dialog);
        QHBoxLayout *layoutLines = new QHBoxLayout();
        QHBoxLayout *layoutGroupBoxes = new QHBoxLayout();
        QHBoxLayout *hLayout = new QHBoxLayout();
        QLabel *labelID = new QLabel("ID: ");
        QLineEdit *lineID = new QLineEdit(ID);
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
                QLineEdit *lineEdit = qobject_cast<QLineEdit*>(layoutItem);
                lineEdit->setText(info.at(i));
            }
        }
        if (dialog.exec() == QDialog::Accepted) {
            QGridLayout *groupInfoLayout = qobject_cast<QGridLayout*>(groupInfo->layout());
            QListWidgetItem *item = new QListWidgetItem();
            item->setText(lineID->text());
            QList<QVariant> data;
            QString model = lineModel->text();
            QString type = comboType->currentText();
            QList<QString> outputs;
            QList<QString> info;
            if (!groupInfoLayout->isEmpty()){
                for(int i = 0; i<groupInfoLayout->rowCount(); i++){
                    QWidget *layoutItem = groupInfoLayout->itemAtPosition(i, 1)->widget();
                    QLineEdit *lineEdit = qobject_cast<QLineEdit*>(layoutItem);
                    if(lineEdit) info << lineEdit->text();
                }
            }
            for(int i = 0; i < listOutputs->count(); i++) {
                outputs << listOutputs->item(i)->text();
            }
            data << model
                 << type
                 << QVariant(outputs)
                 << QVariant(info);
            item->setData(Qt::UserRole, data);
            int row = ui->listWidgetDevices->currentRow();
            delete ui->listWidgetDevices->takeItem(row);
            ui->listWidgetDevices->insertItem(row, item);
        }
    }
}

bool compareVariants(const QVariant& v1, const QVariant& v2)
{
    // Обработка пустых значений
    if (!v1.isValid() && !v2.isValid()) return false;
    if (!v1.isValid()) return true;  // Пустые значения считаем меньшими
    if (!v2.isValid()) return false;

    // Сравниваем с учетом типа данных
    switch (v1.type()) {
    case QVariant::Int:
    case QVariant::UInt:
    case QVariant::LongLong:
    case QVariant::ULongLong:
        if (v2.canConvert<qint64>()) {
            return v1.toLongLong() < v2.toLongLong();
        }
        break;

    case QVariant::Double:
        if (v2.canConvert<double>()) {
            return v1.toDouble() < v2.toDouble();
        }
        break;

    case QVariant::String:
        return v1.toString() < v2.toString();

    case QVariant::DateTime:
        if (v2.canConvert<QDateTime>()) {
            return v1.toDateTime() < v2.toDateTime();
        }
        break;

    case QVariant::Date:
        if (v2.canConvert<QDate>()) {
            return v1.toDate() < v2.toDate();
        }
        break;

    case QVariant::Time:
        if (v2.canConvert<QTime>()) {
            return v1.toTime() < v2.toTime();
        }
        break;

    case QVariant::Bool:
        return v1.toBool() < v2.toBool();

    default:
        // По умолчанию сравниваем как строки
        return v1.toString() < v2.toString();
    }

    // Если не удалось сравнить по типам, сравниваем как строки
    return v1.toString() < v2.toString();
}

void sortListWidgetByUserData(QListWidget* listWidget, int dataIndex, Qt::SortOrder order)
{
    if (!listWidget || listWidget->count() <= 1) return;

    QList<QListWidgetItem*> items;

    // Извлекаем все элементы
    while (listWidget->count() > 0) {
        items.append(listWidget->takeItem(0));
    }

    // Сортируем
    std::sort(items.begin(), items.end(),
              [dataIndex, order](QListWidgetItem* a, QListWidgetItem* b) {

                  QVariant dataA = a->data(Qt::UserRole);
                  QVariant dataB = b->data(Qt::UserRole);

                  QVariant valueA = (dataA.type() == QVariant::List)
                                        ? dataA.toList().value(dataIndex) : dataA;
                  QVariant valueB = (dataB.type() == QVariant::List)
                                        ? dataB.toList().value(dataIndex) : dataB;

                  if (compareVariants(valueA, valueB) == compareVariants(valueB, valueA)){
                      QString textA = a->text();
                      QString textB = b->text();

                      bool less = textA < textB;
                      return order == Qt::AscendingOrder ? less : !less;
                  }
                  else{
                      bool less = compareVariants(valueA, valueB);
                      return order == Qt::AscendingOrder ? less : !less;
                  }
              });

    // Возвращаем в список
    for (QListWidgetItem* item : items) {
        listWidget->addItem(item);
    }
}

void experimentConfigurationDialog::sortDevices()
{
    sortListWidgetByUserData(ui->listWidgetDevices, INDEX_DEVICE_TYPE, devicesSort);
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
        QLineEdit *lineSignalLoss = new QLineEdit();

        groupLayoutNew->addWidget(labelLength, 0, 0);
        groupLayoutNew->addWidget(lineLength, 0, 1);
        groupLayoutNew->addWidget(labelMaterial, 1, 0);
        groupLayoutNew->addWidget(lineMaterial, 1, 1);
        groupLayoutNew->addWidget(labelSignalLoss, 2, 0);
        groupLayoutNew->addWidget(lineSignalLoss, 2, 1);
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
    QListWidgetItem* deviceItem = new QListWidgetItem();
    for (int i = 0; i < ui->listWidgetDevices->count(); ++i) {
        if (ui->listWidgetDevices->item(i)->text() == deviceName){
            deviceItem = ui->listWidgetDevices->item(i);
            break;
        }
    }
    QList<QVariant> data = deviceItem->data(Qt::UserRole).toList();
    QStringList outputs = data.at(INDEX_DEVICE_OUTPUTS).toStringList();
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
        QTableWidgetItem* device1Item = new QTableWidgetItem(comboDevice1->currentText());
        QTableWidgetItem* device2Item = new QTableWidgetItem(comboDevice2->currentText());
        QTableWidgetItem* device1OutputItem = new QTableWidgetItem(comboDeviceOutput1->currentText());
        QTableWidgetItem* device2OutputItem = new QTableWidgetItem(comboDeviceOutput2->currentText());
        QTableWidgetItem* connectionTypeItem = new QTableWidgetItem(comboInterface->currentText());
        QList<QVariant> interfaceData;
        foreach (auto obj, groupSettings->children()) {
            if (qobject_cast<QLineEdit*>(obj)){
                QLineEdit* line = qobject_cast<QLineEdit*>(obj);
                interfaceData.append(QVariant(line->text()));
            }
            else if (qobject_cast<QComboBox*>(obj)){
                QComboBox* combo = qobject_cast<QComboBox*>(obj);
                interfaceData.append(QVariant(combo->currentText()));
            }
        }
        qDebug() << interfaceData;
        connectionTypeItem->setData(Qt::UserRole, interfaceData);
        int row = ui->tableWidgetConnections->rowCount();
        ui->tableWidgetConnections->setRowCount(row + 1);
        ui->tableWidgetConnections->setItem(row, 0, device1Item);
        ui->tableWidgetConnections->setItem(row, 1, device1OutputItem);
        ui->tableWidgetConnections->setItem(row, 2, connectionTypeItem);
        ui->tableWidgetConnections->setItem(row, 3, device2Item);
        ui->tableWidgetConnections->setItem(row, 4, device2OutputItem);
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

    comboDevice1->setCurrentText(ui->tableWidgetConnections->item(row,0)->text());
    comboDevice2->setCurrentText(ui->tableWidgetConnections->item(row,3)->text());
    comboDeviceOutput1->setCurrentText(ui->tableWidgetConnections->item(row,1)->text());
    comboDeviceOutput2->setCurrentText(ui->tableWidgetConnections->item(row,4)->text());
    comboInterface->setCurrentText(ui->tableWidgetConnections->item(row,2)->text());

    QList<QVariant> data = ui->tableWidgetConnections->item(row,2)->data(Qt::UserRole).toList();
    int j = 0;
    foreach (auto obj, groupSettings->children()) {
        if (qobject_cast<QLineEdit*>(obj)){
            QLineEdit* line = qobject_cast<QLineEdit*>(obj);
            line->setText(data.at(j).toString());
            j++;
        }
        else if (qobject_cast<QComboBox*>(obj)){
            QComboBox* combo = qobject_cast<QComboBox*>(obj);
            combo->setCurrentText(data.at(j).toString());
            j++;
        }
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
        QTableWidgetItem* device1Item = new QTableWidgetItem(comboDevice1->currentText());
        QTableWidgetItem* device2Item = new QTableWidgetItem(comboDevice2->currentText());
        QTableWidgetItem* device1OutputItem = new QTableWidgetItem(comboDeviceOutput1->currentText());
        QTableWidgetItem* device2OutputItem = new QTableWidgetItem(comboDeviceOutput2->currentText());
        QTableWidgetItem* connectionTypeItem = new QTableWidgetItem(comboInterface->currentText());
        QList<QVariant> interfaceData;
        foreach (auto obj, groupSettings->children()) {
            if (qobject_cast<QLineEdit*>(obj)){
                QLineEdit* line = qobject_cast<QLineEdit*>(obj);
                interfaceData.append(QVariant(line->text()));
            }
            else if (qobject_cast<QComboBox*>(obj)){
                QComboBox* combo = qobject_cast<QComboBox*>(obj);
                interfaceData.append(QVariant(combo->currentText()));
            }
        }
        qDebug() << interfaceData;
        connectionTypeItem->setData(Qt::UserRole, interfaceData);
        ui->tableWidgetConnections->removeRow(row);
        ui->tableWidgetConnections->insertRow(row);
        ui->tableWidgetConnections->setItem(row, 0, device1Item);
        ui->tableWidgetConnections->setItem(row, 1, device1OutputItem);
        ui->tableWidgetConnections->setItem(row, 2, connectionTypeItem);
        ui->tableWidgetConnections->setItem(row, 3, device2Item);
        ui->tableWidgetConnections->setItem(row, 4, device2OutputItem);
    }
}



void experimentConfigurationDialog::deleteConnection()
{
    if (ui->tableWidgetConnections->selectedItems().isEmpty()){
        QMessageBox::warning(this, "Ошибка", "Устройство не выбрано!");
        return;
    }
    ui->tableWidgetConnections->removeRow(ui->tableWidgetConnections->currentRow());
}

