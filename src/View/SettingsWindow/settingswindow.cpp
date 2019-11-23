#include "settingswindow.h"
#include "ui_settingswindow.h"

// Custom
#include "../src/Model/SettingsManager/SettingsFile.h"

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
}



void SettingsWindow::on_pushButton_clicked()
{
    if (ui ->lineEdit ->text() != "")
    {
       emit signalApply( new SettingsFile( ui ->lineEdit ->text() .toUShort(),
                                           ui ->checkBox_HTML ->isChecked(),
                                           ui ->lineEdit_pass ->text() .toStdWString() ));
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
