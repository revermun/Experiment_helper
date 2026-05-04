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
    explicit eventEditDialog(QMap<QString,eventData>* eventMap, QMap<QString,QPair<QString,QList<QString>>> devicesMap,
                             QMap<QString,Mess> messagesMap, QWidget *parent = nullptr);
    explicit eventEditDialog(QMap<QString,eventData>* eventMap, QMap<QString,QPair<QString,QList<QString>>> devicesMap,
                             QMap<QString,Mess> messagesMap, eventData data, QWidget *parent = nullptr);
    ~eventEditDialog();

    eventData getEventData();

public slots:
    void comboDeviceChangeEvent(QString device);
    void comboMessageChangeEvent(QString message);
    void comboFieldChangeEvent(QString field);
    void eventPresetChangeEvent(QString preset);
    void checkFields();

private:
    bool isEdit;
    Ui::eventEditDialog *ui;
    QMap<QString,QPair<QString,QList<QString>>> devicesMap;
    QMap<QString,Mess> messagesMap;
    QMap<QString,eventData>* eventMap;
};

#endif // EVENTEDITDIALOG_H
