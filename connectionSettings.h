#ifndef CONNECTIONSETTINGS_H
#define CONNECTIONSETTINGS_H

#include <QDialog>
#include <QDebug>
#include <QComboBox>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QPair>
#include <QList>
#include <QSerialPortInfo>

#include "enums.h"
#include "structs.h"

QT_BEGIN_NAMESPACE
namespace Ui { class ConnectionSettings; }
QT_END_NAMESPACE

class ConnectionSettings : public QDialog
{
    Q_OBJECT

public:
    ConnectionSettings(QWidget *parent = nullptr);
    ConnectionSettings(DeviceInfo deviceInfo, QWidget *parent = nullptr);
    ~ConnectionSettings();


    DeviceInfo getSettings();

public slots:
    void changeProtocolComboBox(QString device);
    void changeSettings(QString connectionType);
    void toggleAdressEditable(QString string);
    void saveSettings();
    void checkFields();
    void toggleSerialTCPsettingsEnabled(bool);

private:
    void setChildrenHidden(QObject* parent, bool isHidden);
    Ui::ConnectionSettings *ui;
    DeviceInfo settings;
};
#endif // CONNECTIONSETTINGS_H
