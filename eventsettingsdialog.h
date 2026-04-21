#ifndef EVENTSETTINGSDIALOG_H
#define EVENTSETTINGSDIALOG_H

#include <QDialog>
#include <QMap>

#include "structs.h"

namespace Ui {
class eventSettingsDialog;
}

class eventSettingsDialog : public QDialog
{
    Q_OBJECT

public:
    explicit eventSettingsDialog(QMap<QString,eventData>* eventMap, QMap<QString,QPair<QString,QList<QString>>> devicesMap, QMap<QString,Mess> messagesMap, QWidget *parent = nullptr);
    ~eventSettingsDialog();


public slots:
    void addEvent();
    void editEvent();
    void deleteEvent();


private:
    QMap<QString,eventData>* eventMap;
    QMap<QString,QPair<QString,QList<QString>>> devicesMap;
    QMap<QString,Mess> messagesMap;

    Ui::eventSettingsDialog *ui;
};

#endif // EVENTSETTINGSDIALOG_H
