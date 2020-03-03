// This file is part of the Silent Server.
// Copyright Aleksandr "Flone" Tretyakov (github.com/Flone-dnb).
// Licensed under the ZLib license.
// Refer to the LICENSE file included.

#include "createroomwindow.h"
#include "ui_createroomwindow.h"

CreateRoomWindow::CreateRoomWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::CreateRoomWindow)
{
    ui->setupUi(this);
}

CreateRoomWindow::~CreateRoomWindow()
{
    delete ui;
}
