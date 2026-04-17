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
eventEditDialog::eventEditDialog(QMap<QString,QPair<QString,QList<QString>>> devicesMap, QMap<QString,QPair<QString,QStringList>> fieldsMap, QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::eventEditDialog)
{
    ui->setupUi(this);
    this->devicesMap = devicesMap;
    this->fieldsMap = fieldsMap;
    QStringList devices = devicesMap.keys();
    ui->comboDevice->addItems(devices);

}

eventEditDialog::eventEditDialog(QMap<QString,QPair<QString,QList<QString>>> devicesMap, QMap<QString,QPair<QString,QStringList>> fieldsMap, eventData data, QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::eventEditDialog)
{
    ui->setupUi(this);
    this->devicesMap = devicesMap;
    this->fieldsMap = fieldsMap;
    QStringList devices = devicesMap.keys();
    ui->comboDevice->addItems(devices);
    ui->lineName->setText(data.name);
    ui->comboDevice->setCurrentText(data.device);
    ui->comboMessage->setCurrentText(data.message);
    ui->comboMessageField->setCurrentText(QString::number(data.field));
    ui->comboEventText->setCurrentText(data.text);
    ui->checkLogIfEqual->setChecked(data.triggers.isEqual);
    ui->checkLogIfGreater->setChecked(data.triggers.isGreater);
    ui->checkLogIfLesser->setChecked(data.triggers.isLesser);
    ui->spinThreshhold->setValue(data.triggers.threshhold);
}

eventEditDialog::~eventEditDialog()
{
    delete ui;
}


void eventEditDialog::comboDeviceChangeEvent(QString device){
    QString protocol = devicesMap[device].second.at(INDEX_GENERAL_PROTOCOL);
    QStringList messages;
    foreach (QString mess, fieldsMap.keys()) {
        if (fieldsMap[mess].first == protocol){
            messages.append(mess);
        }
    }
    ui->comboMessage->clear();
    ui->comboMessage->addItems(messages);
}

void eventEditDialog::comboMessageChangeEvent(QString message)
{
    ui->comboMessageField->clear();
    ui->comboMessageField->addItems(fieldsMap[message].second);
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
        data.fieldName = ui->comboMessageField->currentText();
        data.field = fieldsMap[data.message].second.indexOf(data.fieldName);
        data.text = ui->comboEventText->currentText();
        eventData::Triggers triggers;
        triggers.isEqual = ui->checkLogIfEqual->isChecked();
        triggers.isGreater = ui->checkLogIfGreater->isChecked();
        triggers.isLesser = ui->checkLogIfLesser->isChecked();
        triggers.threshhold = ui->spinThreshhold->value();
        data.triggers = triggers;
    }
    return data;

}

