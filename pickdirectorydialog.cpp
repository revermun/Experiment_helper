#include "pickdirectorydialog.h"
#include "ui_pickdirectorydialog.h"

pickDirectoryDialog::pickDirectoryDialog(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::pickDirectoryDialog)
{
    ui->setupUi(this);
}

pickDirectoryDialog::~pickDirectoryDialog()
{
    delete ui;
}

void pickDirectoryDialog::explore(){
    QString dir = QFileDialog::getExistingDirectory(this, tr("Open Directory"),
                                                    "/home",
                                                    QFileDialog::ShowDirsOnly
                                                        | QFileDialog::DontResolveSymlinks);
    if (dir.isEmpty()) return;
    this->dir = dir;
    ui->lineDir->setText(dir);
}

QString pickDirectoryDialog::getDir(){
    return this->dir;
}
