#include "eventeditdialog.h"
#include "ui_eventeditdialog.h"
#include "enums.h"
/// мысли по поводу полей сообщений
/// Без понятия что делать с полями флагов, потому что они могут быть поделены на произвольных размеров фрагменты

/// Идея заключается в том, что индекс названия поля совпадает с индексом этого поля в сообщении, то есть
/// узнав индекс названия можно, имея сообщение, получить и само поле
/// В будущем можно подумать по поводу более умного решения, покрывающего поля флагов,
/// но думаю пока можно остановиться и на этом
/// Можно это сделать конфигурируемым с помощью файла, чтоб не делать bloat code
/// Формат: ID1:protocol:field1,field2,field3\nID2:field1,...
///
/// РЕШЕНИЕ ПРОБЛЕМЫ ПОЛЕЙ И РАБОТЫ ПО ПРОТОКОЛУ - Конфигурационный файл с сообщениями и их полями
/// В файле должна быть исчерпывающая информация о сообщениях и полях
/// Файл будет в формате .xml
/// На первом уровне будут сообщения, у них будут следующие параметры:
/// protocol - Ublox или Unicore (в будущем можно будет дополнить)
/// id - идентификатор сообщения
/// type - conf/nav (для фильтрации во время сбора сообщений для событий)
/// description - короткое описание на русском
/// На втором уровне будут поля сообщений, у них будут следующие параметры:
/// name - развернутое название (до 3х слов)
/// type - int, uint, bitmap/flags, char
/// size - 1,2,4,....(байт)
/// min_value - число (для не int можно оставить пустым)
/// max_value - число (для не int можно оставить пустым)
/// units - единицы измерения (m/s, m, rad, deg, sm/s)
/// scale - число на которое нужно умножить для получения действительного значения (10e-5,1,...)
eventEditDialog::eventEditDialog(QMap<QString,QPair<QString,QList<QString>>> devicesMap, QMap<QString,Mess> messagesMap, QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::eventEditDialog)
{
    ui->setupUi(this);
    this->devicesMap = devicesMap;
    this->messagesMap = messagesMap;
    QStringList devices = devicesMap.keys();
    ui->comboDevice->addItems(devices);
    ui->frameBitmap->setHidden(true);
    ui->frameChar->setHidden(true);
    ui->frameInt->setHidden(true);
}

eventEditDialog::eventEditDialog(QMap<QString,QPair<QString,QList<QString>>> devicesMap, QMap<QString,Mess> messagesMap, eventData data, QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::eventEditDialog)
{
    ui->setupUi(this);
    this->devicesMap = devicesMap;
    this->messagesMap = messagesMap;
    QStringList devices = devicesMap.keys();
    ui->comboDevice->addItems(devices);
    ui->lineName->setText(data.name);
    ui->comboDevice->setCurrentText(data.device);
    ui->comboMessage->setCurrentText(data.message);
    ui->comboMessageField->setCurrentText(data.fieldName);
    ui->comboEventText->setCurrentText(data.text);
    if (data.fieldType == "int" || data.fieldType == "uint" || data.fieldType == "float" || data.fieldType == "double"){
        ui->checkLogIfEqual->setChecked(data.intTriggers.isEqual);
        ui->checkLogIfGreater->setChecked(data.intTriggers.isGreater);
        ui->checkLogIfLesser->setChecked(data.intTriggers.isLesser);
        ui->spinThreshhold->setValue(data.intTriggers.threshhold);
    }
    else if (data.fieldType == "bitmap"){
        ui->spinEndBit->setValue(data.bitmapTriggers.endBit);
        ui->spinStartBit->setValue(data.bitmapTriggers.startBit);
        ui->spinValue->setValue(data.bitmapTriggers.bitValue);
    }
    else if (data.fieldType == "char"){
        ui->lineValue->setText(data.charTriggers.charValue);
    }
}

eventEditDialog::~eventEditDialog()
{
    delete ui;
}


void eventEditDialog::comboDeviceChangeEvent(QString device){
    QString protocol = devicesMap[device].second.at(INDEX_GENERAL_PROTOCOL);
    QStringList messages;
    foreach (QString mess, messagesMap.keys()) {
        if (messagesMap[mess].protocol == protocol){
            if (messagesMap[mess].type != "conf") messages.append(mess);
        }
    }
    ui->comboMessage->clear();
    ui->comboMessage->addItems(messages);
}

void eventEditDialog::comboMessageChangeEvent(QString message)
{
    ui->comboMessageField->clear();
    ui->comboMessageField->addItems(messagesMap[message].getSortedFieldKeys());
}

void eventEditDialog::comboFieldChangeEvent(QString field){
    QString type = messagesMap[ui->comboMessage->currentText()].fields[field].type;
    ui->labelFieldType->setText(type);
    ui->frameBitmap->setHidden(true);
    ui->frameChar->setHidden(true);
    ui->frameInt->setHidden(true);
    if (type == "int" || type == "uint" || type == "float" || type == "double"){
        ui->spinThreshhold->setValue(0);
        ui->frameInt->setHidden(false);
        ui->spinThreshhold->setMinimum(messagesMap[ui->comboMessage->currentText()].fields[field].min_value);
        ui->spinThreshhold->setMaximum(messagesMap[ui->comboMessage->currentText()].fields[field].max_value);
    }
    else if (type == "bitmap"){
        ui->spinEndBit->setValue(0);
        ui->spinStartBit->setValue(0);
        ui->spinValue->setValue(0);
        ui->frameBitmap->setHidden(false);
    }
    else if (type == "char"){
        ui->lineValue->clear();
        ui->frameChar->setHidden(false);
    }
}
void eventEditDialog::checkFields()
{
    if (ui->comboEventType->currentIndex() == -1){
        QMessageBox::warning(this, "Ошибка", "Выберите тип события");
        return;
    }
    if (ui->comboEventType->currentText() == "Кастомное событие"){
        if (ui->lineName->text().isEmpty()){
            QMessageBox::warning(this, "Ошибка", "Введите название события");
            return;
        }
    }
    this->accept();
}

eventData eventEditDialog::getEventData(){
    eventData data;
    if (ui->comboEventType->currentText() == "Кастомное событие"){
        data.name = ui->lineName->text();
        data.device = ui->comboDevice->currentText();
        data.protocol = devicesMap[data.device].second.at(INDEX_GENERAL_PROTOCOL);
        data.message = ui->comboMessage->currentText();
        data.messageId = messagesMap[data.message].id.toInt();
        data.fieldName = ui->comboMessageField->currentText();
        data.field = messagesMap[data.message].fields[data.fieldName].index;
        QString type = messagesMap[data.message].fields[data.fieldName].type;
        data.fieldType = type;
        data.text = ui->comboEventText->currentText();
        if (type == "int" || type == "uint" || type == "float" || type == "double"){
            eventData::IntTriggers triggers;
            triggers.isEqual = ui->checkLogIfEqual->isChecked();
            triggers.isGreater = ui->checkLogIfGreater->isChecked();
            triggers.isLesser = ui->checkLogIfLesser->isChecked();
            triggers.threshhold = ui->spinThreshhold->value();
            data.intTriggers = triggers;
        }
        else if (type == "bitmap"){
            eventData::BitmapTriggers triggers;
            triggers.startBit = ui->spinStartBit->value();
            triggers.endBit = ui->spinEndBit->value();
            triggers.bitValue = ui->spinValue->value();
            data.bitmapTriggers = triggers;
        }
        else if (type == "char"){
            eventData::CharTriggers triggers;
            triggers.charValue = ui->lineValue->text();
            data.charTriggers = triggers;
        }
        data.status = INDEX_FLAGS_UNKNOWN;
    }
    return data;

}

