// This file is part of the Silent Server.
// Copyright Aleksandr "Flone" Tretyakov (github.com/Flone-dnb).
// Licensed under the ZLib license.
// Refer to the LICENSE file included.

#include "aboutqtwindow.h"
#include "ui_aboutqtwindow.h"

AboutQtWindow::AboutQtWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::AboutQtWindow)
{
    ui->setupUi(this);

    setFixedSize (width(), height());
}

AboutQtWindow::~AboutQtWindow()
{
    delete ui;
}

void AboutQtWindow::closeEvent(QCloseEvent *event)
{
    Q_UNUSED(event)

    deleteLater();
}
