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
                             QMap<QString,QPair<QString,QStringList>> fieldsMap, QWidget *parent = nullptr);
    explicit eventEditDialog(QMap<QString,QPair<QString,QList<QString>>> devicesMap,
                             QMap<QString,QPair<QString,QStringList>> fieldsMap, eventData data, QWidget *parent = nullptr);
    ~eventEditDialog();

    eventData getEventData();

public slots:
    void comboDeviceChangeEvent(QString device);
    void comboMessageChangeEvent(QString message);
    void checkFields();

private:
    Ui::eventEditDialog *ui;
    QMap<QString,QPair<QString,QList<QString>>> devicesMap;
    QMap<QString,QPair<QString,QStringList>> fieldsMap;
};

#endif // EVENTEDITDIALOG_H
