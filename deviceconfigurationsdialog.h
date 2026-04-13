#ifndef DEVICECONFIGURATIONSDIALOG_H
#define DEVICECONFIGURATIONSDIALOG_H

#include <QDialog>
#include <QSerialPort>
#include <QMap>
#include <QDebug>
#include <QtConcurrent/QtConcurrent>
#include <QTreeWidgetItem>
#include <QTableWidget>

namespace Ui {
class deviceConfigurationsDialog;
}

class deviceConfigurationsDialog : public QDialog
{
    Q_OBJECT

public:
    explicit deviceConfigurationsDialog(QMap<QString,QPair<QString,QList<QString>>> devicesMap,QMap<QString, QSerialPort*> connectionsMap, QWidget *parent = nullptr);
    ~deviceConfigurationsDialog();

public slots:
    void sendSettings();
    void updateSettings();
    void deviceChangeEvent();
    void tabChangeEvent();
    void updateMessageSettings(QTreeWidgetItem* item, int column);
    void sendSet();
    void sendPoll();
    void saveConfig();
    void pollConfigMessages(QString msgType);
    void parseMessage();
    void changeListsSelection();
    void changeBBRSelection(QString text);
    void enableNAV5AltLines(QString text);
    void showPortSettings(QString text);
    void enableITFMSettings(bool state);
    void enableRelposFrame(bool state);
    void enablePosFrame(bool state);
    void enableVelFrame(bool state);
    void enableRawFrame(bool state);
    void enableEphFrame(bool state);
    void MASKcheckClickEvent(bool state);
    void comboCONFIGRTKchangeEvent(QString text);

private:
    void setupTableSize(QTableWidget* table);
    void setChildrenHidden(QObject* parent, bool isHidden);
    void setChildrenEnabled(QObject* parent, bool isEnabled);
    void sendMSGPoll(uint8_t classID, uint8_t messageID);
    QMap<QString,QPair<QString,QList<QString>>> devicesMap;
    QMap<QString, QSerialPort*> connectionsMap;
    QMap<QByteArray,QString> messagesNamesMap;
    QMap<QString, QPair<uint8_t,uint8_t>> messagesIDMapUBX;
    QMap<QString, QFrame*> framesMap;
    QMap<QString, QString> messagesDescriptionsMap;
    QSerialPort* currentConnection = nullptr;
    QString currentItemText;
    QByteArray streamBuffer;
    QString protocol;
    int count = 0;
    bool velCheck;
    bool posCheck = false;

    Ui::deviceConfigurationsDialog *ui;
};

#endif // DEVICECONFIGURATIONSDIALOG_H
