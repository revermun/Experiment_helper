#ifndef PRESETSETTINGSDIALOG_H
#define PRESETSETTINGSDIALOG_H

#include <QDialog>

#include "structs.h"

namespace Ui {
class presetSettingsDialog;
}

class presetSettingsDialog : public QDialog
{
    Q_OBJECT

public:
    explicit presetSettingsDialog(QWidget *parent = nullptr, QString title = "", QString description = "");
    ~presetSettingsDialog();

    Preset getPreset();

private:
    Ui::presetSettingsDialog *ui;
};

#endif // PRESETSETTINGSDIALOG_H
