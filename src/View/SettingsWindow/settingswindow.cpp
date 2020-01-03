// This file is part of the Silent Server.
// Copyright Aleksandr "Flone" Tretyakov (github.com/Flone-dnb).
// Licensed under the ZLib license.
// Refer to the LICENSE file included.

#include "settingswindow.h"
#include "ui_settingswindow.h"


// Qt
#include <QFileDialog>
#include <QMessageBox>

// Custom
#include "Model/SettingsManager/SettingsFile.h"

SettingsWindow::SettingsWindow(QWidget *parent) : QMainWindow(parent), ui(new Ui::SettingsWindow)
{
    ui->setupUi(this);

    setFixedSize(width(), height());
}

void SettingsWindow::setSettings( SettingsFile* pSettingsFile )
{
    ui ->lineEdit ->setText ( QString::number(pSettingsFile ->iPort) );

    ui ->checkBox_HTML ->setChecked( pSettingsFile ->bAllowHTMLInMessages );

    ui ->lineEdit_pass ->setText( QString::fromStdWString( pSettingsFile ->sPasswordToJoin ) );

    if ( pSettingsFile ->bLog )
    {
        ui ->checkBox_log ->setChecked(true);
        ui ->lineEdit_log_file_path ->setEnabled(true);
        ui ->pushButton_log_file_select_path ->setEnabled(true);

        ui ->lineEdit_log_file_path ->setText( QString::fromStdWString( pSettingsFile ->sPathToLogFile ) );
    }
}



void SettingsWindow::on_pushButton_clicked()
{
    if (ui ->lineEdit ->text() != "")
    {
        if (ui ->checkBox_log ->isChecked() == false)
        {
            ui ->lineEdit_log_file_path ->setText("");
        }
        else
        {
            if ( ui ->lineEdit_log_file_path ->text() == "" )
            {
                ui ->checkBox_log ->setChecked(false);
                ui ->lineEdit_log_file_path ->setText("");
            }
        }

        emit signalApply( new SettingsFile( ui ->lineEdit ->text() .toUShort(),
                                            ui ->checkBox_HTML ->isChecked(),
                                            ui ->lineEdit_pass ->text() .toStdWString(),
                                            ui ->checkBox_log ->isChecked(),
                                            ui ->lineEdit_log_file_path ->text() .toStdWString() ));
    }

    close();
}

void SettingsWindow::closeEvent(QCloseEvent *ev)
{
    Q_UNUSED(ev)

    deleteLater();
}

void SettingsWindow::on_pushButton_log_file_select_path_clicked()
{
    QString sPathToLogFile = QFileDialog::getSaveFileName(this, "Select the Path",
                                                          "", "Text File (*.txt)");

    if (sPathToLogFile .size() > UCHAR_MAX)
    {
        QMessageBox::warning(nullptr, "Path Too Long", "Cannot save current settings. The path to the selected file is too long.");
    }
    else
    {
        ui ->lineEdit_log_file_path ->setText(sPathToLogFile);
    }
}

void SettingsWindow::on_checkBox_log_stateChanged(int arg1)
{
    if ( ui ->checkBox_log ->isChecked() )
    {
        ui ->lineEdit_log_file_path ->setEnabled(true);
        ui ->pushButton_log_file_select_path ->setEnabled(true);
    }
    else
    {
        ui ->lineEdit_log_file_path ->setEnabled(false);
        ui ->pushButton_log_file_select_path ->setEnabled(false);
    }
}

SettingsWindow::~SettingsWindow()
{
    delete ui;
}
