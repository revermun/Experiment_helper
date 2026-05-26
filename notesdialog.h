#ifndef NOTESDIALOG_H
#define NOTESDIALOG_H

#include <QDialog>
#include <QDebug>
#include <QPropertyAnimation>
#include <QDir>

#include "structs.h"
#include "enums.h"

namespace Ui {
class notesDialog;
}

class notesDialog : public QDialog
{
    Q_OBJECT

public:
    explicit notesDialog(QString experimentDirectory, bool isLap, QWidget *parent = nullptr);
    ~notesDialog();

    QList<Note> getNotes();
    void changeDir(QString dir);
    void saveNotes();

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
    void loadNotes();
    void animateResize(int newWidth, int newHeight);
    QList<Note> notesList;

    QString experimentDirectory;
};

#endif // NOTESDIALOG_H
