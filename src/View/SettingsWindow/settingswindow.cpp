#include "settingswindow.h"
#include "ui_settingswindow.h"

SettingsWindow::SettingsWindow(QWidget *parent) : QMainWindow(parent), ui(new Ui::SettingsWindow)
{
    ui->setupUi(this);

    setFixedSize(width(), height());
}

void SettingsWindow::setSettings(QString sServerPort)
{
    ui ->lineEdit ->setText (sServerPort);
}



void SettingsWindow::on_pushButton_clicked()
{
    if (ui ->lineEdit ->text() != "")
    {
       emit signalApply( ui ->lineEdit ->text() .toUShort() );
    }

    close();
}

void SettingsWindow::closeEvent(QCloseEvent *ev)
{
    deleteLater();
}


SettingsWindow::~SettingsWindow()
{
    delete ui;
}
