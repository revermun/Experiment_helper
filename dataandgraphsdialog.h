#ifndef DATAANDGRAPHSDIALOG_H
#define DATAANDGRAPHSDIALOG_H

#include <QDialog>
#include <QLabel>
#include <QDebug>
#include <QGroupBox>
#include <QLineEdit>
#include <QComboBox>
#include <QSerialPort>
#include <QTimer>
#include <QMessageBox>

/// TODO: когда будет реализовано подлючение организовать реализацию графиков
namespace Ui {
class dataAndGraphsDialog;
}

class dataAndGraphsDialog : public QDialog
{
    Q_OBJECT

public:
    explicit dataAndGraphsDialog(QMap<QString,QPair<QString,QList<QString>>> devicesMap,QMap<QString, QObject*> connectionsMap, QWidget *parent = nullptr);
    ~dataAndGraphsDialog();

public slots:
    void addGroup();
    void openSettings();
    void deleteGroup();
    void openGraphs();
    void changeDataComboBox(QComboBox* combo);
    void changeUnitComboBox(QComboBox* combo);
    void sendASCII();
    void sendNum();
    void openCloseStream();
    void writeToTerminal();

private:
    QGroupBox* addGroupBox();
    void removeItem(QGroupBox* itemToRemove);
    void rearrangeGrid();

    QMap<QString,QPair<QString,QList<QString>>> devicesMap;
    QMap<QString, QObject*> connectionsMap;
    QVector<QPair<QGroupBox*, QPair<int, int>>> groupBoxVector;
    Ui::dataAndGraphsDialog *ui;

    // QWidgetList groupBoxVector;
    bool streamIsOpen = false;
    int nextRow = 0;
    int nextColumn = 0;
};

#endif // DATAANDGRAPHSDIALOG_H
