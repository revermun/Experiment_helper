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

/// TODO: когда будет реализовано подлючение организовать вывод в строки данных, терминал и реализацию графиков
/// TODO: терминал https://github.com/lxqt/qtermwidget/releases
namespace Ui {
class dataAndGraphsDialog;
}

class dataAndGraphsDialog : public QDialog
{
    Q_OBJECT

public:
    explicit dataAndGraphsDialog(QMap<QString,QPair<QString,QList<QString>>> devicesMap,QMap<QString, QSerialPort*> connectionsMap, QWidget *parent = nullptr);
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
    QMap<QString, QSerialPort*> connectionsMap;
    QVector<QPair<QGroupBox*, QPair<int, int>>> groupBoxVector;
    Ui::dataAndGraphsDialog *ui;

    // QWidgetList groupBoxVector;
    bool streamIsOpen = false;
    int nextRow = 0;
    int nextColumn = 0;
};

#endif // DATAANDGRAPHSDIALOG_H
