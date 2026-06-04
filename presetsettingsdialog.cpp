#include "presetsettingsdialog.h"
#include "ui_presetsettingsdialog.h"

presetSettingsDialog::presetSettingsDialog(QWidget *parent, QString title, QString description)
    : QDialog(parent)
    , ui(new Ui::presetSettingsDialog)
{
    ui->setupUi(this);
    ui->lineTitle->setText(title);
    ui->textDescription->setText(description);
}

presetSettingsDialog::~presetSettingsDialog()
{
    delete ui;
}


Preset presetSettingsDialog::getPreset()
{
    Preset preset;
    preset.title = ui->lineTitle->text();
    preset.description = ui->textDescription->toPlainText();
    return preset;
}
