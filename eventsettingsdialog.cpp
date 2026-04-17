#include "eventsettingsdialog.h"
#include "ui_eventsettingsdialog.h"
#include "eventeditdialog.h"

eventSettingsDialog::eventSettingsDialog(QMap<QString,eventData>* eventMap,QMap<QString,QPair<QString,QList<QString>>> devicesMap, QMap<QString,QPair<QString,QStringList>> fieldsMap, QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::eventSettingsDialog)
{
    ui->setupUi(this);
    this->eventMap = eventMap;
    this->devicesMap = devicesMap;
    this->fieldsMap = fieldsMap;
    foreach (QString key, eventMap->keys()) {
        eventData data = eventMap->value(key);
        QVariant name =      QVariant(data.name);
        QVariant device =    QVariant(data.device);
        QVariant protocol =  QVariant(data.protocol);
        QVariant message =   QVariant(data.message);
        QVariant fieldName = QVariant(data.fieldName);
        QVariant field =     QVariant(data.field);
        QVariant text =      QVariant(data.text);
        QVariant isGreater =    QVariant(data.triggers.isGreater);
        QVariant isLesser =     QVariant(data.triggers.isLesser);
        QVariant isEqual =      QVariant(data.triggers.isEqual);
        QVariant threshhold =    QVariant(data.triggers.threshhold);
        eventMap->insert(key, data);
        QListWidgetItem* item = new QListWidgetItem;
        item->setText(data.name);
        QList<QVariant> variantList = {name, device, protocol, message, fieldName, field, text, isGreater, isLesser, isEqual, threshhold};
        item->setData(Qt::UserRole, variantList);
        ui->listEvents->addItem(item);
    }
}

eventSettingsDialog::~eventSettingsDialog()
{
    delete ui;
}

void eventSettingsDialog::addEvent()
{
    eventEditDialog eED = eventEditDialog(devicesMap, fieldsMap, this);
    if (eED.exec() == QDialog::Accepted){
        eventData data = eED.getEventData();
        QVariant name =      QVariant(data.name);
        QVariant device =    QVariant(data.device);
        QVariant protocol =  QVariant(data.protocol);
        QVariant message =   QVariant(data.message);
        QVariant fieldName = QVariant(data.fieldName);
        QVariant field =     QVariant(data.field);
        QVariant text =      QVariant(data.text);
        QVariant isGreater =    QVariant(data.triggers.isGreater);
        QVariant isLesser =     QVariant(data.triggers.isLesser);
        QVariant isEqual =      QVariant(data.triggers.isEqual);
        QVariant threshhold =    QVariant(data.triggers.threshhold);
        eventMap->insert(data.name, data);
        QListWidgetItem* item = new QListWidgetItem;
        item->setText(data.name);
        QList<QVariant> variantList = {name, device, protocol, message, fieldName, field, text, isGreater, isLesser, isEqual, threshhold};
        item->setData(Qt::UserRole, variantList);
        ui->listEvents->addItem(item);
    }
}

void eventSettingsDialog::editEvent()
{
    if (ui->listEvents->selectedItems().isEmpty()) {
        QMessageBox::warning(this, "Ошибка", "Выберите событие");
        return;
    }
    QListWidgetItem* item = ui->listEvents->currentItem();
    QList<QVariant> variantList = item->data(Qt::UserRole).toList();
    eventData dataOld;
    dataOld.name = variantList.at(0).toString();
    dataOld.device = variantList.at(1).toString();
    dataOld.protocol = variantList.at(2).toString();
    dataOld.message = variantList.at(3).toString();
    dataOld.fieldName = variantList.at(4).toString();
    dataOld.field = variantList.at(5).toInt();
    dataOld.text = variantList.at(6).toString();
    dataOld.triggers.isGreater = variantList.at(7).toBool();
    dataOld.triggers.isLesser = variantList.at(8).toBool();
    dataOld.triggers.isEqual = variantList.at(9).toBool();
    dataOld.triggers.threshhold = variantList.at(10).toInt();
    eventEditDialog eED = eventEditDialog(devicesMap, fieldsMap, dataOld, this);
    if (eED.exec() == QDialog::Accepted){
        eventData data = eED.getEventData();
        QVariant name =      QVariant(data.name);
        QVariant device =    QVariant(data.device);
        QVariant protocol =  QVariant(data.protocol);
        QVariant message =   QVariant(data.message);
        QVariant fieldName = QVariant(data.fieldName);
        QVariant field =     QVariant(data.field);
        QVariant text =      QVariant(data.text);
        QVariant isGreater =    QVariant(data.triggers.isGreater);
        QVariant isLesser =     QVariant(data.triggers.isLesser);
        QVariant isEqual =      QVariant(data.triggers.isEqual);
        QVariant threshhold =    QVariant(data.triggers.threshhold);
        eventMap->remove(data.name);
        eventMap->insert(data.name, data);
        item->setText(data.name);
        QList<QVariant> variantList = {name, device, protocol, message, fieldName, field, text, isGreater, isLesser, isEqual, threshhold};
        item->setData(Qt::UserRole, variantList);
    }
}

void eventSettingsDialog::deleteEvent()
{
    if (ui->listEvents->selectedItems().isEmpty()) {
        QMessageBox::warning(this, "Ошибка", "Выберите событие");
        return;
    }
    QString name = ui->listEvents->currentItem()->text();
    ui->listEvents->takeItem(ui->listEvents->currentRow());
    eventMap->remove(name);
}
