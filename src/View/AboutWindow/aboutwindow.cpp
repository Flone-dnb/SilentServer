// This file is part of the Silent Server.
// Copyright Aleksandr "Flone" Tretyakov (github.com/Flone-dnb).
// Licensed under the ZLib license.
// Refer to the LICENSE file included.

#include "aboutwindow.h"
#include "ui_aboutwindow.h"

// Qt
#include <QDesktopServices>
#include <QUrl>

AboutWindow::AboutWindow(QString sSilentVersion, QString sSilentLastClientVersion, QWidget *parent) : QMainWindow(parent), ui(new Ui::AboutWindow)
{
    ui->setupUi(this);
    setFixedSize ( width (), height () );


    ui ->label_appIcon       ->setPixmap ( QPixmap(":/appMainIcon.png").scaled (128, 128, Qt::KeepAspectRatio) );
    ui ->label_silentVersion ->setText   ( "Silent Server. Version: " + sSilentVersion + ".\n"
                                           "Supported client version: " + sSilentLastClientVersion + "." );
    ui ->label_copyright     ->setText   ( "Copyright (c) 2019-2020.\nAleksandr \"Flone\" Tretyakov." );
}



void AboutWindow::closeEvent(QCloseEvent *pEvent)
{
    deleteLater ();
}




void AboutWindow::on_pushButton_clicked()
{
    QDesktopServices::openUrl (QUrl("https://github.com/Flone-dnb/SilentServer"));
}


AboutWindow::~AboutWindow()
{
    delete ui;
}
