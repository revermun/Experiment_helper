#include "eventsettingsdialog.h"
#include "ui_eventsettingsdialog.h"
#include "eventeditdialog.h"

eventSettingsDialog::eventSettingsDialog(QMap<QString,eventData>* eventMap,QMap<QString,QPair<QString,QList<QString>>> devicesMap, QMap<QString,Mess> messagesMap, QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::eventSettingsDialog)
{
    ui->setupUi(this);
    this->eventMap = eventMap;
    this->devicesMap = devicesMap;
    this->messagesMap = messagesMap;
    foreach (QString key, eventMap->keys()) {
        eventData data = eventMap->value(key);
        QVariant name =      QVariant(data.name);
        QVariant device =    QVariant(data.device);
        QVariant protocol =  QVariant(data.protocol);
        QVariant message =   QVariant(data.message);
        QVariant fieldName = QVariant(data.fieldName);
        QVariant field =     QVariant(data.field);
        QVariant fieldType = QVariant(data.fieldType);
        QVariant text =      QVariant(data.text);
        QVariant isGreater =    QVariant(data.intTriggers.isGreater);
        QVariant isLesser =     QVariant(data.intTriggers.isLesser);
        QVariant isEqual =      QVariant(data.intTriggers.isEqual);
        QVariant threshhold =    QVariant(data.intTriggers.threshhold);
        QVariant startBit =    QVariant(data.bitmapTriggers.startBit);
        QVariant endBit =     QVariant(data.bitmapTriggers.endBit);
        QVariant bitValue =      QVariant(data.bitmapTriggers.value);
        QVariant charValue =    QVariant(data.charTriggers.value);
        eventMap->insert(data.name,data);
        QListWidgetItem* item = new QListWidgetItem;
        item->setText(data.name);
        QList<QVariant> variantList = {name, device, protocol, message, fieldName, field, fieldType, text, isGreater, isLesser, isEqual, threshhold, startBit, endBit, bitValue, charValue};
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
    eventEditDialog eED = eventEditDialog(devicesMap, messagesMap, this);
    if (eED.exec() == QDialog::Accepted){
        eventData data = eED.getEventData();
        QVariant name =      QVariant(data.name);
        QVariant device =    QVariant(data.device);
        QVariant protocol =  QVariant(data.protocol);
        QVariant message =   QVariant(data.message);
        QVariant fieldName = QVariant(data.fieldName);
        QVariant field =     QVariant(data.field);
        QVariant fieldType = QVariant(data.fieldType);
        QVariant text =      QVariant(data.text);
        QVariant isGreater =    QVariant(data.intTriggers.isGreater);
        QVariant isLesser =     QVariant(data.intTriggers.isLesser);
        QVariant isEqual =      QVariant(data.intTriggers.isEqual);
        QVariant threshhold =    QVariant(data.intTriggers.threshhold);
        QVariant startBit =    QVariant(data.bitmapTriggers.startBit);
        QVariant endBit =     QVariant(data.bitmapTriggers.endBit);
        QVariant bitValue =      QVariant(data.bitmapTriggers.value);
        QVariant charValue =    QVariant(data.charTriggers.value);
        eventMap->insert(data.name,data);
        QListWidgetItem* item = new QListWidgetItem;
        item->setText(data.name);
        QList<QVariant> variantList = {name, device, protocol, message, fieldName, field, fieldType, text, isGreater, isLesser, isEqual, threshhold, startBit, endBit, bitValue, charValue};
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
    dataOld.fieldType = variantList.at(6).toString();
    dataOld.text = variantList.at(7).toString();
    dataOld.intTriggers.isGreater = variantList.at(8).toBool();
    dataOld.intTriggers.isLesser = variantList.at(9).toBool();
    dataOld.intTriggers.isEqual = variantList.at(10).toBool();
    dataOld.intTriggers.threshhold = variantList.at(11).toInt();
    dataOld.bitmapTriggers.startBit = variantList.at(12).toInt();
    dataOld.bitmapTriggers.endBit = variantList.at(13).toInt();
    dataOld.bitmapTriggers.value = variantList.at(14).toInt();
    dataOld.charTriggers.value = variantList.at(15).toString();
    eventEditDialog eED = eventEditDialog(devicesMap, messagesMap, dataOld, this);
    if (eED.exec() == QDialog::Accepted){
        eventData data = eED.getEventData();
        QVariant name =      QVariant(data.name);
        QVariant device =    QVariant(data.device);
        QVariant protocol =  QVariant(data.protocol);
        QVariant message =   QVariant(data.message);
        QVariant fieldName = QVariant(data.fieldName);
        QVariant field =     QVariant(data.field);
        QVariant fieldType = QVariant(data.fieldType);
        QVariant text =      QVariant(data.text);
        QVariant isGreater =    QVariant(data.intTriggers.isGreater);
        QVariant isLesser =     QVariant(data.intTriggers.isLesser);
        QVariant isEqual =      QVariant(data.intTriggers.isEqual);
        QVariant threshhold =    QVariant(data.intTriggers.threshhold);
        QVariant startBit =    QVariant(data.bitmapTriggers.startBit);
        QVariant endBit =     QVariant(data.bitmapTriggers.endBit);
        QVariant bitValue =      QVariant(data.bitmapTriggers.value);
        QVariant charValue =    QVariant(data.charTriggers.value);
        eventMap->remove(dataOld.name);
        eventMap->insert(data.name,data);
        item->setText(data.name);
        QList<QVariant> variantList = {name, device, protocol, message, fieldName, field, fieldType, text, isGreater, isLesser, isEqual, threshhold, startBit, endBit, bitValue, charValue};
        item->setData(Qt::UserRole, variantList);
        ui->listEvents->addItem(item);
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
