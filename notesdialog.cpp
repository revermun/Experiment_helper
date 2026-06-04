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
    this->experimentDirectory = experimentDirectory;
    loadNotes();
}


notesDialog::~notesDialog()
{
    saveNotes();
    delete ui;
}

void notesDialog::loadNotes(){
    QDir directory(experimentDirectory + '/' + "Notes" + '/');
    if (!directory.exists()) {
        directory.mkpath(".");
    }

    QStringList fileNames = directory.entryList(QDir::Files | QDir::NoDotAndDotDot);

    for(QString fileName : fileNames){
        QStringList fragments = fileName.split("_");
        if(fragments.count() < 3){
            continue;
        }

        QString tag = fragments.at(0);
        QString title;

        /// Если имеется дело с general, то название файла имеет 3 части: тэг, General и заголовок
        if (fragments.at(1) == "General"){
            title = fragments.at(2).split(".").at(0);
        }
        /// Если имеется дело с Lap, то название файла имеет 4 части: тэг, Lap, номер захода и заголовок
        else if (fragments.at(1) == "Lap"){
            title = fragments.at(3).split(".").at(0);
        }

        QFile file(experimentDirectory + '/' + "Notes" + '/' + fileName);
        if(!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
            qDebug() << "Failed to open file:" << fileName;
            continue;
        }

        QTextStream stream(&file);
        stream.setCodec("UTF-8");

        QString body = stream.readAll();

        qDebug() << "Loaded note:" << title << "Body length:" << body.length();
        file.close();

        QListWidgetItem* item = new QListWidgetItem();
        Note note;
        note.tag = tag;
        note.body = body;
        note.title = title;
        notesList.append(note);
        item->setText('(' + tag + ") " + title);
        ui->listWidgetNotes->addItem(item);
    }
}
void notesDialog::changeDir(QString dir)
{
    this->experimentDirectory = dir;
}

void notesDialog::saveNotes()
{
    QDir directory(experimentDirectory + '/' + "Notes" + '/');

    if (!directory.exists()) {
        directory.mkpath(".");
    }

    QStringList fileNamesOld = directory.entryList(QDir::Files | QDir::NoDotAndDotDot);
    for(QString fileNameOld : fileNamesOld){
        QFile::remove(experimentDirectory + "/Notes/" + fileNameOld);
    }

    for (const Note& note : std::as_const(notesList)){
        QString tag = note.tag;
        QString title = note.title;
        QString body = note.body;

        QString safeTitle = title;
        safeTitle.remove(QRegularExpression("[\\\\/*?:\"<>|]"));

        QString fileName = tag + "_General_" + safeTitle + ".txt";

        QFile file(experimentDirectory + "/Notes/" + fileName);
        if(!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
            qDebug() << "Failed to open file for writing:" << fileName;
            continue;
        }

        QTextStream stream(&file);
        stream.setCodec("UTF-8");
        stream << body;
        file.close();
    }
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
        int index = ui->listWidgetNotes->currentRow();
        Note note = notesList.at(index);
        ui->comboBoxTag->setCurrentText(note.tag);
        ui->lineEditTitle->setText(note.title);
        ui->textEditNote->setText(note.body);
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
        Note note = notesList.at(row);
        QString itemTag = note.tag;
        QString itemTitle = note.title;
        if (itemTag == tag && itemTitle == title){
            ui->labelWarning->setText("Заметка с таким названием и тэгом уже существует!");
            return;
        }
    }
    QString body = ui->textEditNote->toPlainText();
    qDebug() << body;
    Note note;
    note.tag = tag;
    note.title = title;
    note.body = body;
    QString newItemTitle = '(' + tag + ") " + title;
    if (isAdd){
        QListWidgetItem* item = new QListWidgetItem(newItemTitle);
        ui->listWidgetNotes->addItem(item);
        notesList.append(note);
    }
    else if (isEdit){
        int row = ui->listWidgetNotes->currentRow();
        notesList.removeAt(row);
        notesList.insert(row,note);
        QListWidgetItem* oldItem = ui->listWidgetNotes->item(row);
        oldItem->setText(newItemTitle);
    }
    ui->listWidgetNotes->setEnabled(true);
    closeRedactor();
}

void notesDialog::deleteNote()
{
    if (ui->listWidgetNotes->count()>0 && !ui->groupBoxRedactor->isVisible() && !ui->listWidgetNotes->selectedItems().isEmpty()) {
        int row = ui->listWidgetNotes->currentRow();
        notesList.removeAt(row);
        delete ui->listWidgetNotes->takeItem(ui->listWidgetNotes->currentRow());
    }
}

void notesDialog::filterNotes(QString tag)
{
    for(int row = 0; row<ui->listWidgetNotes->count(); row++){
        QListWidgetItem* item = ui->listWidgetNotes->item(row);
        Note note = notesList.at(row);
        QString itemTag = note.tag;
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
    ui->listWidgetNotes->setEnabled(true);
    this->animateResize(noRedactorWidth, noRedactorHeight);
}

void notesDialog::openRedactor()
{
    noRedactorHeight = this->height();
    noRedactorWidth = this->width();
    ui->groupBoxRedactor->setVisible(true);
}

QList<Note> notesDialog::getNotes()
{
    return notesList;
}
