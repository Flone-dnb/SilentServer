#include "settingswindow.h"
#include "ui_settingswindow.h"

// Custom
#include "../src/Model/SettingsManager/SettingsFile.h"

SettingsWindow::SettingsWindow(SettingsFile* pSettingsFile, QWidget *parent) : QMainWindow(parent), ui(new Ui::SettingsWindow)
{
    ui->setupUi(this);

    setFixedSize(width(), height());

    setSettings( QString::number( pSettingsFile ->iPort ) );
}

void SettingsWindow::setSettings(QString sServerPort)
{
    ui ->lineEdit ->setText (sServerPort);
}



void SettingsWindow::on_pushButton_clicked()
{
    if (ui ->lineEdit ->text() != "")
    {
       emit signalApply( new SettingsFile(ui ->lineEdit ->text() .toUShort()) );
    }

    close();
}

void SettingsWindow::closeEvent(QCloseEvent *ev)
{
    Q_UNUSED(ev)

    deleteLater();
}


SettingsWindow::~SettingsWindow()
{
    delete ui;
}
