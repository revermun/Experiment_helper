#ifndef PICKDIRECTORYDIALOG_H
#define PICKDIRECTORYDIALOG_H

#include <QDialog>
#include <QFileDialog>

namespace Ui {
class pickDirectoryDialog;
}

class pickDirectoryDialog : public QDialog
{
    Q_OBJECT

public:
    explicit pickDirectoryDialog(QWidget *parent = nullptr);
    ~pickDirectoryDialog();

    QString getDir();

public slots:
    void explore();

private:
    QString dir;
    Ui::pickDirectoryDialog *ui;
};

#endif // PICKDIRECTORYDIALOG_H
