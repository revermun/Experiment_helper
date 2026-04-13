#include "dataandgraphsdialog.h"
#include "ui_dataandgraphsdialog.h"
#include "enums.h"

dataAndGraphsDialog::dataAndGraphsDialog(QMap<QString,QPair<QString,QList<QString>>> devicesMap,QMap<QString, QSerialPort*> connectionsMap, QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::dataAndGraphsDialog)
{
    ui->setupUi(this);
    this->devicesMap = devicesMap;
    this->connectionsMap = connectionsMap;
    // groupBoxVector.append(ui->groupBox);
    QPalette palette = ui->textEditTerminal->palette();
    palette.setColor(QPalette::Text, Qt::white);
    palette.setColor(QPalette::Base, Qt::black);

    QStringList keys = connectionsMap.keys();
    foreach (QString key, keys) {
        ui->comboBoxDevice->addItem(key);
    }
    ui->comboBoxDevice->setCurrentIndex(-1);
    QTimer *timer = new QTimer(this);
    connect(timer, SIGNAL(timeout()), this, SLOT(writeToTerminal()));
    timer->start(1);

    ui->textEditTerminal->setPalette(palette);
}

dataAndGraphsDialog::~dataAndGraphsDialog()
{
    delete ui;
}

QGroupBox* dataAndGraphsDialog::addGroupBox()
{
    QGroupBox* newGroupBox = new QGroupBox("Новая группа");
    newGroupBox->setAlignment(Qt::AlignHCenter);
    QVBoxLayout* layout = new QVBoxLayout(newGroupBox);
    QHBoxLayout* btnLayout = new QHBoxLayout();
    QPushButton* settingsButton = new QPushButton(newGroupBox);
    settingsButton->setStyleSheet("image: url(:/resources/settings.png);");
    QPushButton* deleteButton = new QPushButton(newGroupBox);
    deleteButton->setStyleSheet("image: url(:/resources/remove.png);");
    QPushButton* graphButton = new QPushButton(newGroupBox);
    graphButton->setStyleSheet("image: url(:/resources/graph.png);");

    QObject::connect(settingsButton, SIGNAL(clicked()), this, SLOT(openSettings()));
    QObject::connect(deleteButton, SIGNAL(clicked()), this, SLOT(deleteGroup()));
    QObject::connect(graphButton, SIGNAL(clicked()), this, SLOT(openGraphs()));

    btnLayout->addWidget(settingsButton);
    btnLayout->addWidget(deleteButton);
    btnLayout->addWidget(graphButton);

    layout->addLayout(btnLayout);

    newGroupBox->setLayout(layout);
    return newGroupBox;
}

void dataAndGraphsDialog::deleteGroup()
{
    QPushButton* removeButton = qobject_cast<QPushButton*>(sender());
    if (!removeButton) return;

    QGroupBox* groupBox = qobject_cast<QGroupBox*>(removeButton->parent());
    if (!groupBox) return;

    // Удаляем этот groupBox
    removeItem(groupBox);
}

void dataAndGraphsDialog::removeItem(QGroupBox* itemToRemove)
{
    // Находим индекс удаляемого элемента в векторе
    int removeIndex = -1;
    for (int i = 0; i < groupBoxVector.size(); ++i) {
        if (groupBoxVector[i].first == itemToRemove) {
            removeIndex = i;
            break;
        }
    }

    if (removeIndex == -1) return;

    // Удаляем из сетки и освобождаем память
    ui->gridLayoutGroupBoxes->removeWidget(itemToRemove);
    delete itemToRemove;

    // Удаляем из вектора
    groupBoxVector.remove(removeIndex);

    // Перестраиваем сетку
    rearrangeGrid();
}

void dataAndGraphsDialog::rearrangeGrid()
{
    // Удаляем addGroupBox из сетки
    ui->gridLayoutGroupBoxes->removeWidget(ui->groupBox);

    // Очищаем сетку от всех элементов
    for (auto& item : groupBoxVector) {
        ui->gridLayoutGroupBoxes->removeWidget(item.first);
    }

    // Заново расставляем все элементы в правильном порядке
    nextRow = 0;
    nextColumn = 0;

    for (int i = 0; i < groupBoxVector.size(); ++i) {
        // Добавляем элемент в текущую позицию
        ui->gridLayoutGroupBoxes->addWidget(groupBoxVector[i].first, nextRow, nextColumn);

        // Обновляем сохраненную позицию
        groupBoxVector[i].second = qMakePair(nextRow, nextColumn);

        // Переходим к следующей колонке
        if(nextColumn == 1){
            nextRow++;
            nextColumn = 0;
        }
        else{
            nextColumn++;
        }
    }

    // Добавляем addGroupBox в конец
    ui->gridLayoutGroupBoxes->addWidget(ui->groupBox, nextRow, nextColumn);
}
/// в теории можно намутить формулы https://github.com/Aleksandr185/MathExpressions
void dataAndGraphsDialog::changeUnitComboBox(QComboBox* combo)
{
    QComboBox *comboData = qobject_cast<QComboBox*>(sender());
    combo->clear();
    QString data = comboData->currentText();
    if (data.isEmpty()){
        return;
    }

    if (data.toLower().contains("угол") || data.toLower().contains("крен") || data.toLower().contains("тангаж")){
        combo->addItem("Градусы");
        combo->addItem("Радианы");
    }
    else if (data.toLower().contains("широта") || data.toLower().contains("долгота")){
        combo->addItem("Градусы");
    }
    else if (data.toLower().contains("скорость")){
        combo->addItem("Метры в секунду");
    }
    else if (data.toLower().contains("ускорение")){
        combo->addItem("Метры в секунду**2");
    }
    else if (data.toLower().contains("координат") || data.toLower().contains("длина") || data.toLower().contains("ширина") || data.toLower().contains("расстояние")){
        combo->addItem("Метры");
        combo->addItem("Сантиметры");
    }
    else{
        combo->addItem("Единицы");
    }

}

void dataAndGraphsDialog::openGraphs()
{
    QGroupBox* groupBox = qobject_cast<QGroupBox*>(sender()->parent());
    QVBoxLayout* groupLayout = qobject_cast<QVBoxLayout*>(groupBox->layout());
    if (groupLayout->count()==1){
        QMessageBox::warning(this, "Отсутствуют данные!", "Настройте данные!");
        return;
    }

    QDialog dialog(this);
    dialog.setWindowTitle("Конструктор графика");
    QHBoxLayout *layout = new QHBoxLayout(&dialog);
    QVBoxLayout *layoutLeft = new QVBoxLayout();
    QVBoxLayout *layoutRight = new QVBoxLayout();
    QLabel *labelType = new QLabel("Наименование группы");
    QComboBox *comboType = new QComboBox();
    comboType->addItem("Измерения");
    comboType->addItem("Разность двух измерений от времени");
    QHBoxLayout *typeLayout = new QHBoxLayout();
    QGroupBox *dataGroupBox = new QGroupBox();
    dataGroupBox->setAlignment(Qt::AlignHCenter);
    QGridLayout* dataGroupBoxLayout = new QGridLayout();

    QLabel *labelUnits = new QLabel("Единицы измерения");
    QLabel *labelData = new QLabel("Данные");
    labelUnits->setSizePolicy(QSizePolicy::Preferred,QSizePolicy::Maximum);
    QComboBox *comboData1 = new QComboBox();
    QComboBox *comboUnit1 = new QComboBox();
    QComboBox *comboData2 = new QComboBox();
    QComboBox *comboUnit2 = new QComboBox();
    QComboBox *comboData3 = new QComboBox();
    QComboBox *comboUnit3 = new QComboBox();
    QComboBox *comboData4 = new QComboBox();
    QComboBox *comboUnit4 = new QComboBox();
    QVector<QComboBox*> vectorData = {comboData1,comboData2,comboData3, comboData4};

    foreach (QComboBox* combo, vectorData) {
        combo->addItem("");
    }
    connect(comboData1, &QComboBox::currentTextChanged, this, [this, comboUnit1](const QString& text) {changeUnitComboBox(comboUnit1);});
    connect(comboData2, &QComboBox::currentTextChanged, this, [this, comboUnit2](const QString& text) {changeUnitComboBox(comboUnit2);});
    connect(comboData3, &QComboBox::currentTextChanged, this, [this, comboUnit3](const QString& text) {changeUnitComboBox(comboUnit3);});
    connect(comboData4, &QComboBox::currentTextChanged, this, [this, comboUnit4](const QString& text) {changeUnitComboBox(comboUnit4);});

    // QStringList dataList;
    QGridLayout* groupGridLayout = qobject_cast<QGridLayout*>(groupLayout->itemAt(1)->layout());
    for (int i = 0; i<groupGridLayout->rowCount(); i++){
        QLabel *labelDataID = qobject_cast<QLabel*>(groupGridLayout->itemAtPosition(i,0)->widget());
        QString dataString = labelDataID->text();
        // dataList << dataString;
        foreach (QComboBox* combo, vectorData) {
            combo->addItem(dataString.split("\n").at(1));
        }
    }
    dataGroupBoxLayout->addWidget(labelData,0,0);
    dataGroupBoxLayout->addWidget(labelUnits,0,1);
    dataGroupBoxLayout->addWidget(comboData1,1,0);
    dataGroupBoxLayout->addWidget(comboUnit1,1,1);
    dataGroupBoxLayout->addWidget(comboData2,2,0);
    dataGroupBoxLayout->addWidget(comboUnit2,2,1);
    dataGroupBoxLayout->addWidget(comboData3,3,0);
    dataGroupBoxLayout->addWidget(comboUnit3,3,1);
    dataGroupBoxLayout->addWidget(comboData4,4,0);
    dataGroupBoxLayout->addWidget(comboUnit4,4,1);

    dataGroupBox->setLayout(dataGroupBoxLayout);
    ///

    QLabel *labelTitle = new QLabel("Заголовок графика");
    QLineEdit *lineTitle = new QLineEdit();
    QLabel *labelX = new QLabel("Ось X:");
    QLineEdit *lineX = new QLineEdit();
    QLabel *labelY = new QLabel("Ось Y:");
    QHBoxLayout *YLayout = new QHBoxLayout();
    QLabel *labelYTime = new QLabel("Время:");
    QComboBox *comboYTime = new QComboBox();
    comboYTime->addItem("Секунды");
    comboYTime->addItem("Милисекунды");
    QHBoxLayout *hLayout = new QHBoxLayout();
    QPushButton *okButton = new QPushButton("Готово");
    QPushButton *cancelButton = new QPushButton("Отмена");

    typeLayout->addWidget(labelType);
    typeLayout->addWidget(comboType);

    layoutLeft->addLayout(typeLayout);
    layoutLeft->addWidget(dataGroupBox);

    layoutRight->addWidget(labelTitle);
    layoutRight->addWidget(lineTitle);
    layoutRight->addWidget(labelX);
    layoutRight->addWidget(lineX);
    layoutRight->addWidget(labelY);

    YLayout->addWidget(labelYTime);
    YLayout->addWidget(comboYTime);

    layoutRight->addLayout(YLayout);
    layoutRight->addSpacing(100);

    hLayout->addStretch(100);
    hLayout->addWidget(okButton);
    hLayout->addWidget(cancelButton);
    hLayout->addStretch(100);

    layoutRight->addLayout(hLayout);

    layout->addLayout(layoutLeft);
    layout->addLayout(layoutRight);

    QObject::connect(okButton, &QPushButton::clicked, &dialog, &QDialog::accept);
    QObject::connect(cancelButton, &QPushButton::clicked, &dialog, &QDialog::reject);

    if (dialog.exec() == QDialog::Accepted) {
        /// TODO: сдесь должен быть код реалицации открытия отдельного окна с графиком
    }
}

void dataAndGraphsDialog::changeDataComboBox(QComboBox* combo)
{
    QComboBox *comboDevice = qobject_cast<QComboBox*>(sender());
    combo->clear();
    QString deviceID = comboDevice->currentText();
    if (deviceID.isEmpty()){
        return;
    }
    QString device = devicesMap[deviceID].second[INDEX_GENERAL_DEVICE_TYPE];
    if (device == "Приемник"){
        /// TOASK: спросить конкретные названия данных
        combo->addItem("Координаты позиции");
        combo->addItem("Кол-во видимых спутников");
        combo->addItem("Среднее отношение сигнал/шум");
        combo->addItem("Скорость");
        combo->addItem("Крен");
        combo->addItem("Тангаж");
        combo->addItem("Длина вектора базы");
    }
    else if (device == "IMU"){
        combo->addItem("Ускорение");
        combo->addItem("Скорость");
        combo->addItem("Крен");
        combo->addItem("Тангаж");
    }
    else if (device == "Другое"){
        //пока не понятно
    }
}

void clearLayout(QLayout* layout)
{
    if (!layout) return;

    QLayoutItem* child;
    while ((child = layout->takeAt(0))) {
        if (child->layout()) {
            clearLayout(child->layout());
            delete child->layout();
        }
        if (child->widget()) {
            delete child->widget();
        }
        delete child;
    }
}

void dataAndGraphsDialog::openSettings()
{
    QGroupBox* groupBox = qobject_cast<QGroupBox*>(sender()->parent());

    QDialog dialog(this);
    dialog.setWindowTitle("Настройка Группы");
    dialog.setFixedHeight(347);
    QVBoxLayout *layout = new QVBoxLayout(&dialog);

    QLabel *labelID = new QLabel("Наименование группы");
    QLineEdit *lineID = new QLineEdit(groupBox->title());
    QGridLayout *gridLayout = new QGridLayout();
    QComboBox *comboData1 = new QComboBox();
    QComboBox *comboDevice1 = new QComboBox();
    QComboBox *comboData2 = new QComboBox();
    QComboBox *comboDevice2 = new QComboBox();
    QComboBox *comboData3 = new QComboBox();
    QComboBox *comboDevice3 = new QComboBox();
    QComboBox *comboData4 = new QComboBox();
    QComboBox *comboDevice4 = new QComboBox();

    QVector<QComboBox*> vectorDevice = {comboDevice1, comboDevice2, comboDevice3, comboDevice4};
    QVector<QComboBox*> vectorData = {comboData1, comboData2, comboData3, comboData4};
    foreach(QComboBox* combo, vectorDevice){
        QStringList keys = devicesMap.keys();
        foreach (QString key, keys) {
            combo->addItem(key);
        }
        combo->setCurrentIndex(-1);
    }

    gridLayout->addWidget(comboData1,0,0);
    gridLayout->addWidget(comboDevice1,0,1);
    gridLayout->addWidget(comboData2,1,0);
    gridLayout->addWidget(comboDevice2,1,1);
    gridLayout->addWidget(comboData3,2,0);
    gridLayout->addWidget(comboDevice3,2,1);
    gridLayout->addWidget(comboData4,3,0);
    gridLayout->addWidget(comboDevice4,3,1);

    QHBoxLayout *hLayout = new QHBoxLayout();
    QPushButton *okButton = new QPushButton("Готово");
    QPushButton *cancelButton = new QPushButton("Отмена");

    hLayout->addStretch(100);
    hLayout->addWidget(okButton);
    hLayout->addWidget(cancelButton);
    hLayout->addStretch(100);

    layout->addWidget(labelID);
    layout->addWidget(lineID);
    layout->addLayout(gridLayout);
    layout->addLayout(hLayout);

    connect(comboDevice1, &QComboBox::currentTextChanged, this, [this, comboData1](const QString& text) {changeDataComboBox(comboData1);});
    connect(comboDevice2, &QComboBox::currentTextChanged, this, [this, comboData2](const QString& text) {changeDataComboBox(comboData2);});
    connect(comboDevice3, &QComboBox::currentTextChanged, this, [this, comboData3](const QString& text) {changeDataComboBox(comboData3);});
    connect(comboDevice4, &QComboBox::currentTextChanged, this, [this, comboData4](const QString& text) {changeDataComboBox(comboData4);});
    QObject::connect(okButton, &QPushButton::clicked, &dialog, &QDialog::accept);
    QObject::connect(cancelButton, &QPushButton::clicked, &dialog, &QDialog::reject);

    if (dialog.exec() == QDialog::Accepted) {
        groupBox->setTitle(lineID->text());
        QGridLayout* groupGridLayout = new QGridLayout();
        QStringList dataTitles;
        for(int i = 0; i < vectorData.size(); i++){
            if(!vectorDevice.at(i)->currentText().isEmpty()){
                QString dataTitle = '(' + vectorDevice.at(i)->currentText() + ")\n" + vectorData.at(i)->currentText();
                dataTitles.append(dataTitle);
            }
        }
        for(int i = 0; i<dataTitles.size(); i++){
            QLabel* label = new QLabel(dataTitles.at(i));
            QLineEdit* lineEdit = new QLineEdit();
            groupGridLayout->addWidget(label,i,0);
            groupGridLayout->addWidget(lineEdit,i,1);
        }
        QVBoxLayout* layout = qobject_cast<QVBoxLayout*>(groupBox->layout());
        if (layout->count() > 1){
            QLayoutItem* item = layout->takeAt(layout->count() - 1);
            clearLayout(item->layout());
        }
        layout->addLayout(groupGridLayout);
    }
}

void dataAndGraphsDialog::addGroup(){
    ui->gridLayoutGroupBoxes->removeWidget(ui->groupBox);

    QGroupBox* newGroupBox = addGroupBox();
    groupBoxVector.append(qMakePair(newGroupBox, qMakePair(nextRow, nextColumn)));
    ui->gridLayoutGroupBoxes->addWidget(newGroupBox, nextRow, nextColumn);

    if(nextColumn == 1){
        nextRow++;
        nextColumn = 0;
    }
    else{
        nextColumn++;
    }
    qDebug() << nextRow << nextColumn;
    ui->gridLayoutGroupBoxes->addWidget(ui->groupBox, nextRow, nextColumn);
}

/// TOASK:: можно ли во время открытого потока менять устройство

/// При записи в терминал нужно очищать потоки всех устройств кроме выбранного, если поток отрыт мы пишем, если нет то очищаем.
/// (пока что при открытом потоке мы будем блокировать замену устройства)

void dataAndGraphsDialog::writeToTerminal()
{
    QByteArray stream;
    foreach (QString key, connectionsMap.keys()) {
        QSerialPort* connection = connectionsMap[key];
        if (key != ui->comboBoxDevice->currentText()){
            connection->clear();
            continue;
        }
        if (!(streamIsOpen && ui->comboBoxDevice->currentIndex()!= -1)){
            connection->clear();
            continue;
        }

        if (ui->comboBoxOutputType->currentText() == "Hex + space"){
            while(connection->waitForReadyRead(1)){
                stream.append(connection->readAll());
                if(stream.isEmpty()){
                    break;
                }
                stream = stream.toHex(' ');
                ui->textEditTerminal->append(stream);
                ui->textEditTerminal->append("");
            }
        }
        else{
            if (connection->canReadLine()) {
                stream.append(connection->readLine(1024));
                if(stream.isEmpty()){
                    break;
                }
                std::string qmessage = stream.toStdString();
                ui->textEditTerminal->append(QString::fromStdString(qmessage));
                ui->textEditTerminal->append("");
            }
        }
    }
}

void dataAndGraphsDialog::openCloseStream()
{
    streamIsOpen = !streamIsOpen;
    ui->comboBoxDevice->setEnabled(!streamIsOpen);
}

QByteArray stringToNumFormat(const QString &input)
{
    QByteArray result;

    // Удаляем все пробелы и приводим к верхнему регистру для удобства
    QString cleanInput = input.simplified().remove(' ').toUpper();

    int i = 0;
    while (i < cleanInput.length()) {
        // Проверяем, является ли текущий фрагмент hex-значением вида "0xA"
        if (cleanInput[i] == '0' && i + 2 < cleanInput.length() &&
            (cleanInput[i + 1] == 'X' || cleanInput[i + 1] == 'x')) {

            // Извлекаем hex-цифру после "0x"
            QChar hexChar = cleanInput[i + 2];

            // Проверяем, является ли символ допустимой hex-цифрой
            if (hexChar.isDigit() || (hexChar >= 'A' && hexChar <= 'F')) {
                bool ok;
                // Преобразуем hex-символ в число
                int value = QString(hexChar).toInt(&ok, 16);
                if (ok) {
                    result.append(static_cast<char>(value));
                    i += 3; // Пропускаем "0x" и hex-цифру
                    continue;
                }
            }
        }

        // Обычный символ - преобразуем в число (для цифр от 0 до 9)
        if (cleanInput[i].isDigit()) {
            bool ok;
            int value = QString(cleanInput[i]).toInt(&ok);
            if (ok && value >= 0 && value <= 9) {
                result.append(static_cast<char>(value));
                i++;
                continue;
            }
        }

        // Если символ не удалось обработать, добавляем предупреждение
        qWarning() << "Не удалось обработать символ:" << cleanInput[i];
        i++;
    }

    return result;
}


/// В ASCII что написал, то и отсылается
/// В Num написанное преобразовывается в hex числа
void dataAndGraphsDialog::sendNum()
{
    QString mess = ui->lineEditMessageToSend->text();
    QByteArray num = stringToNumFormat(mess);
    qDebug() << num;
    qDebug() << num.toHex();
    QSerialPort* connection = connectionsMap[ui->comboBoxDevice->currentText()];
    if (ui->checkBoxSlashR){
        num += '\r';
    }
    if (ui->checkBoxSlashN){
        num += '\n';
    }
    connection->write(num);
}

void dataAndGraphsDialog::sendASCII()
{
    QString mess = ui->lineEditMessageToSend->text();
    QByteArray ascii = mess.toLatin1();
    QSerialPort* connection = connectionsMap[ui->comboBoxDevice->currentText()];
    if (ui->checkBoxSlashR){
        ascii += '\r';
    }
    if (ui->checkBoxSlashN){
        ascii += '\n';
    }
    qDebug() << ascii;
    qDebug() << ascii.toHex();
    connection->write(ascii);
}

//uniloglist
//unlog

