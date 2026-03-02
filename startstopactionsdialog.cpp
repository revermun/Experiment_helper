#include "startstopactionsdialog.h"
#include "ui_startstopactionsdialog.h"

/// TODO: Просмотр файлов
///
/// TODO: Понять куда сохраняются конфиги после завершения работы с окном и сохраняются ли они вообще
///
/// TODO: Определить можно ли менять конфиг на старте и стопе для одного и того же устройства

startStopActionsDialog::startStopActionsDialog(QList<QString> devices,QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::startStopActionsDialog)
{
    ui->setupUi(this);
    ui->comboBoxDevices->addItems(devices);
}

startStopActionsDialog::~startStopActionsDialog()
{
    delete ui;
}

void startStopActionsDialog::exploreConfig()
{
    QString fileName = QFileDialog::getOpenFileName(this, tr("Open File"),
                                                    "/home",
                                                    tr("Config files (*.conf)"));
    if (fileName == "") {return;}
    qDebug() << fileName;
    QString confName = fileName.split('/').last();
    ui->lineEditConfName->setText(confName);
}

void startStopActionsDialog::addItemToStart(int row, QString ID, QString config)
{
    QListWidgetItem* item = new QListWidgetItem();
    QList<QVariant> data = {QVariant(ID), QVariant(config)};
    item->setText(ID + " \t|\t "+ config);
    item->setData(Qt::UserRole,data);
    ui->listWidgetStart->insertItem(row, item);
}

void startStopActionsDialog::addItemToStop(int row, QString ID, QString config)
{
    QListWidgetItem* item = new QListWidgetItem();
    QList<QVariant> data = {QVariant(ID), QVariant(config)};
    item->setText(ID + " \t|\t "+ config);
    item->setData(Qt::UserRole,data);
    ui->listWidgetStop->insertItem(row, item);
}

void startStopActionsDialog::addItemToStart(QString ID, QString config)
{
    QListWidgetItem* item = new QListWidgetItem();
    QList<QVariant> data = {QVariant(ID), QVariant(config)};
    item->setText(ID + " \t|\t "+ config);
    item->setData(Qt::UserRole,data);
    ui->listWidgetStart->addItem(item);
}

void startStopActionsDialog::addItemToStop(QString ID, QString config)
{
    QListWidgetItem* item = new QListWidgetItem();
    QList<QVariant> data = {QVariant(ID), QVariant(config)};
    item->setText(ID + " \t|\t "+ config);
    item->setData(Qt::UserRole,data);
    ui->listWidgetStop->addItem(item);
}

void startStopActionsDialog::addToStart()
{
    QString ID = ui->comboBoxDevices->currentText();
    QString config = ui->lineEditConfName->text();
    for(int row = 0; row<ui->listWidgetStart->count(); row++){
        QListWidgetItem* item = ui->listWidgetStart->item(row);
        QString itemID = item->data(Qt::UserRole).toList()[INDEX_START_STOP_ID].toString();
        if (itemID == ID){
            QDialog dialog(this);
            dialog.setWindowTitle("Ошибка!");
            dialog.setFixedSize(600, 250);

            QVBoxLayout *layout = new QVBoxLayout(&dialog);

            QLabel *label = new QLabel("Для выбранного устройства уже добавлен конфигурационный файл!\nЗаменить?");

            QPushButton *okButton = new QPushButton("Да");
            QPushButton *cancelButton = new QPushButton("Нет");

            layout->addWidget(label);

            QHBoxLayout *subLayout = new QHBoxLayout(&dialog);

            subLayout->addWidget(okButton);
            subLayout->addWidget(cancelButton);
            layout->addLayout(subLayout);

            QObject::connect(okButton, &QPushButton::clicked, &dialog, &QDialog::accept);
            QObject::connect(cancelButton, &QPushButton::clicked, &dialog, &QDialog::reject);

            // Модальное выполнение
            if (dialog.exec() == QDialog::Accepted) {
                delete ui->listWidgetStart->takeItem(row);
                addItemToStart(row,ID,config);
                return;
            }
            else return;
        }
    }
    addItemToStart(ID,config);
}
void startStopActionsDialog::addToStop()
{
    QString ID = ui->comboBoxDevices->currentText();
    QString config = ui->lineEditConfName->text();
    for(int row = 0; row<ui->listWidgetStop->count(); row++){
        QListWidgetItem* item = ui->listWidgetStop->item(row);
        QString itemID = item->data(Qt::UserRole).toList()[INDEX_START_STOP_ID].toString();
        if (itemID == ID){
            QDialog dialog(this);
            dialog.setWindowTitle("Ошибка!");
            dialog.setFixedSize(600, 250);

            QVBoxLayout *layout = new QVBoxLayout(&dialog);

            QLabel *label = new QLabel("Для выбранного устройства уже добавлен конфигурационный файл!\nЗаменить?");

            QPushButton *okButton = new QPushButton("Да");
            QPushButton *cancelButton = new QPushButton("Нет");

            layout->addWidget(label);

            QHBoxLayout *subLayout = new QHBoxLayout(&dialog);

            subLayout->addWidget(okButton);
            subLayout->addWidget(cancelButton);
            layout->addLayout(subLayout);

            QObject::connect(okButton, &QPushButton::clicked, &dialog, &QDialog::accept);
            QObject::connect(cancelButton, &QPushButton::clicked, &dialog, &QDialog::reject);

            // Модальное выполнение
            if (dialog.exec() == QDialog::Accepted) {
                delete ui->listWidgetStop->takeItem(row);
                addItemToStart(row,ID,config);
                return;
            }
            else return;
        }
    }
    addItemToStop(ID,config);
}
void startStopActionsDialog::removeStart()
{
    if (!ui->listWidgetStart->selectedItems().isEmpty() && ui->listWidgetStart->count()>0) {
        delete ui->listWidgetStart->takeItem(ui->listWidgetStart->currentRow());
    }
}

void startStopActionsDialog::removeStop()
{
    if (!ui->listWidgetStop->selectedItems().isEmpty() && ui->listWidgetStop->count()>0){
        delete ui->listWidgetStop->takeItem(ui->listWidgetStop->currentRow());
    }
}

