#ifndef EVENTEDITDIALOG_H
#define EVENTEDITDIALOG_H

#include <QDialog>
#include <QMap>
#include <QDebug>
#include <QMessageBox>

#include "structs.h"

namespace Ui {
class eventEditDialog;
}

class eventEditDialog : public QDialog
{
    Q_OBJECT

public:
    explicit eventEditDialog(QMap<QString,QPair<QString,QList<QString>>> devicesMap,
                             QMap<QString,Mess> messagesMap, QWidget *parent = nullptr);
    explicit eventEditDialog(QMap<QString,QPair<QString,QList<QString>>> devicesMap,
                             QMap<QString,Mess> messagesMap, eventData data, QWidget *parent = nullptr);
    ~eventEditDialog();

    eventData getEventData();

public slots:
    void comboDeviceChangeEvent(QString device);
    void comboMessageChangeEvent(QString message);
    void comboFieldChangeEvent(QString field);
    void checkFields();

private:
    Ui::eventEditDialog *ui;
    QMap<QString,QPair<QString,QList<QString>>> devicesMap;
    QMap<QString,Mess> messagesMap;
};

#endif // EVENTEDITDIALOG_H
