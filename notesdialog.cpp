#include "notesdialog.h"
#include "ui_notesdialog.h"

/// Функция работы с заметками: создание, редактирование и удаление. Заметки состоят из заголовка,
/// основного текста (описания) и тега/типа. Хранятся в виде файлов в соответствующей папке.
/// Названия файлов формируются как: <Тег>_<Lap_N или General>_<Заголовок> (N – номер захода).
/// Теги заметок: Initial – дополнительная информация о сетапе эксперимента; Error – информация
/// о возникающих ошибках; Change – изменения в сетапе эксперимента, Info – любые заметки.
/// Теги выставляются пользователем при формировании/редактировании заметки.
/// Второй префикс <Lap_N или General> определяется по моменту создания заметки.
/// Если во время захода, то ставится <Lap_N>, иначе <General>.


notesDialog::notesDialog(QString experimentDirectory, bool isLap, QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::notesDialog)
{
    ui->setupUi(this);
    ui->groupBoxRedactor->setVisible(false);
    noRedactorHeight = this->height();
    noRedactorWidth = this->width();
    QDir directory(experimentDirectory + '/' + "Notes" + '/');
    QStringList fileNames = directory.entryList(QDir::AllEntries | QDir::NoDotAndDotDot | QDir::Hidden);
    qDebug() << fileNames;
    for(QString fileName: fileNames){
        QStringList fragments = fileName.split("_");
        if(fragments.count() < 3){
            continue;
        }
        QString tag = fragments.at(0);
        QString title;
        QString body;
        /// Если имеется дело с general, то название файла имеет 3 части: тэг, General и заголовок
        if (fragments.at(1) == "General"){
            title = fragments.at(2).split(".").at(0);
        }
        /// Если имеется дело с Lap, то название файла имеет 4 части: тэг, Lap, номер захода и заголовок
        else if (fragments.at(1) == "Lap"){
            title = fragments.at(3).split(".").at(0);
        }
        QFile file(experimentDirectory + '/' + "Notes" + '/' + fileName);
        if( !file.open( QIODevice::ReadOnly | QIODevice::Text ) )
        {
            qDebug( "Failed to open file for writing." );
        }
        QTextStream stream( &file );
        stream.setCodec("UTF-8");
        stream >> body;
        file.close();
        QListWidgetItem* item = new QListWidgetItem();
        QList<QVariant> data = {QVariant(tag), QVariant(title), QVariant(body), QVariant(isLap)};
        item->setText('(' + tag + ") " + title);
        item->setData(Qt::UserRole,data);
        ui->listWidgetNotes->addItem(item);

    }
}


notesDialog::~notesDialog()
{
    delete ui;
}


void notesDialog::animateResize(int newWidth, int newHeight)
{
    QPropertyAnimation *animation = new QPropertyAnimation(this, "size");
    animation->setDuration(300); // 300 мс
    animation->setEasingCurve(QEasingCurve::InOutQuad);

    animation->setStartValue(this->size());
    animation->setEndValue(QSize(newWidth, newHeight));

    animation->start(QAbstractAnimation::DeleteWhenStopped);
}

void notesDialog::addNote()
{
    if (!ui->groupBoxRedactor->isVisible()){
        openRedactor();
        isAdd = true;
        isEdit = false;
    }
}

void notesDialog::editNote()
{
    if (!ui->groupBoxRedactor->isVisible() && ui->listWidgetNotes->count()>0 && !ui->listWidgetNotes->selectedItems().isEmpty()){
        openRedactor();
        isAdd = false;
        isEdit = true;
        ui->comboBoxTag->setCurrentText(ui->listWidgetNotes->currentItem()->data(Qt::UserRole).toList()[0].toString());
        ui->lineEditTitle->setText(ui->listWidgetNotes->currentItem()->data(Qt::UserRole).toList()[1].toString());
        ui->textEditNote->setText(ui->listWidgetNotes->currentItem()->data(Qt::UserRole).toList()[2].toString());
        ui->listWidgetNotes->setEnabled(false);
    }
}

bool checkValidSymbols(QString string)
{
    if (string.contains('/') || string.contains('\\') || string.contains('|') || string.contains('<')
        || string.contains('>') || string.contains('*') || string.contains('"') || string.contains('?')
        || string.contains(':') || string.contains('_')){
        return false;
    }
    else return true;
}

void notesDialog::redactorAccept()
{
    ui->labelWarning->clear();
    QString tag = ui->comboBoxTag->currentText();
    QString title = ui->lineEditTitle->text();
    if (!checkValidSymbols(title))
    {
        ui->labelWarning->setText("Обнаружены недопустимые символы в заголовке!");
        return;
    }
    for(int row = 0; row<ui->listWidgetNotes->count(); row++){
        if (row == ui->listWidgetNotes->currentRow()){
            continue;
        }
        QListWidgetItem* item = ui->listWidgetNotes->item(row);
        QString itemTag = item->data(Qt::UserRole).toList()[INDEX_NOTE_TAG].toString();
        QString itemTitle = item->data(Qt::UserRole).toList()[INDEX_NOTE_TITLE].toString();
        if (itemTag == tag && itemTitle == title){
            ui->labelWarning->setText("Заметка с таким названием и тэгом уже существует!");
            return;
        }
    }
    QString body = ui->textEditNote->toPlainText();

    QListWidgetItem* item = new QListWidgetItem();
    QList<QVariant> data = {QVariant(tag), QVariant(title), QVariant(body), QVariant(isLap)};
    item->setText('(' + tag + ") " + title);
    item->setData(Qt::UserRole,data);
    if (isAdd){
        ui->listWidgetNotes->addItem(item);
    }
    else if (isEdit){
        int row = ui->listWidgetNotes->currentRow();
        delete ui->listWidgetNotes->takeItem(row);
        ui->listWidgetNotes->insertItem(row, item);
    }
    ui->listWidgetNotes->setEnabled(true);
    closeRedactor();
}

void notesDialog::deleteNote()
{
    if (ui->listWidgetNotes->count()>0 && !ui->groupBoxRedactor->isVisible() && !ui->listWidgetNotes->selectedItems().isEmpty()) {
        delete ui->listWidgetNotes->takeItem(ui->listWidgetNotes->currentRow());
    }
}

void notesDialog::filterNotes(QString tag)
{
    for(int row = 0; row<ui->listWidgetNotes->count(); row++){
        QListWidgetItem* item = ui->listWidgetNotes->item(row);
        QString itemTag = item->data(Qt::UserRole).toList()[0].toString();
        if(tag == "All"){
            item->setHidden(false);
        }
        else if(itemTag == tag){
            item->setHidden(false);
        }
        else item->setHidden(true);
    }
}

void notesDialog::closeRedactor()
{
    ui->groupBoxRedactor->setVisible(false);
    ui->lineEditTitle->clear();
    ui->textEditNote->clear();
    ui->comboBoxTag->setCurrentIndex(0);
    this->animateResize(noRedactorWidth, noRedactorHeight);
}

void notesDialog::openRedactor()
{
    noRedactorHeight = this->height();
    noRedactorWidth = this->width();
    ui->groupBoxRedactor->setVisible(true);
}

QList<QList<QString>> notesDialog::getNotes()
{
    QList<QList<QString>> notesList;
    for(int row = 0; row<ui->listWidgetNotes->count(); row++){
        QListWidgetItem* item = ui->listWidgetNotes->item(row);
        QString itemTag = item->data(Qt::UserRole).toList()[INDEX_NOTE_TAG].toString();
        QString itemTitle = item->data(Qt::UserRole).toList()[INDEX_NOTE_TITLE].toString();
        QString itemBody = item->data(Qt::UserRole).toList()[INDEX_NOTE_BODY].toString();
        QString itemIsLap = item->data(Qt::UserRole).toList()[INDEX_NOTE_ISLAP].toString();
        QList<QString> note;
        note << itemTag << itemTitle << itemBody << itemIsLap;
        notesList << note;
    }
    return notesList;
}
