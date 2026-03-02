#ifndef NOTESDIALOG_H
#define NOTESDIALOG_H

#include <QDialog>
#include <QDebug>
#include <QPropertyAnimation>
#include <QDir>
/// TODO: Когда будет настроено подключение начать реализовывать передачу данных
enum notesIdexes{
    INDEX_NOTE_TAG,
    INDEX_NOTE_TITLE,
    INDEX_NOTE_BODY,
    INDEX_NOTE_ISLAP
};

namespace Ui {
class notesDialog;
}

class notesDialog : public QDialog
{
    Q_OBJECT

public:
    explicit notesDialog(QString experimentDirectory, bool isLap, QWidget *parent = nullptr);
    ~notesDialog();

    QList<QList<QString>> getNotes();

public slots:
    void addNote();
    void editNote();
    void deleteNote();
    void filterNotes(QString tag);
    void openRedactor();
    void closeRedactor();
    void redactorAccept();


private:
    Ui::notesDialog *ui;
    bool isAdd;
    bool isEdit;
    bool isLap;
    int noRedactorWidth;
    int noRedactorHeight;
    int redactorWidth;
    int redactorHeight;
    void animateResize(int newWidth, int newHeight);
};

#endif // NOTESDIALOG_H
