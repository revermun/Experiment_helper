#ifndef STARTSTOPACTIONSDIALOG_H
#define STARTSTOPACTIONSDIALOG_H

#include <QDialog>
#include <QFileDialog>
#include <QDebug>

enum startStopActionsIndexes{
    INDEX_START_STOP_ID,
    INDEX_START_STOP_CONFIG
};

namespace Ui {
class startStopActionsDialog;
}

class startStopActionsDialog : public QDialog
{
    Q_OBJECT

public:
    explicit startStopActionsDialog(QList<QString> devices, QWidget *parent = nullptr);
    ~startStopActionsDialog();

public slots:
    void addToStart();
    void addToStop();
    void removeStart();
    void removeStop();
    void exploreConfig();
private:
    Ui::startStopActionsDialog *ui;
    QList<QString> devices;
    void addItemToStart(int row, QString ID, QString config);
    void addItemToStop(int row, QString ID, QString config);
    void addItemToStart(QString ID, QString config);
    void addItemToStop(QString ID, QString config);
};

#endif // STARTSTOPACTIONSDIALOG_H
