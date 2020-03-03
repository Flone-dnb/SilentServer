// This file is part of the Silent Server.
// Copyright Aleksandr "Flone" Tretyakov (github.com/Flone-dnb).
// Licensed under the ZLib license.
// Refer to the LICENSE file included.

#include "createroomwindow.h"
#include "ui_createroomwindow.h"

#include <QKeyEvent>
#include <QMessageBox>

#include "View/CustomList/SListWidget/slistwidget.h"

CreateRoomWindow::CreateRoomWindow(SListWidget* pList, QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::CreateRoomWindow)
{
    ui->setupUi(this);

    this->pList = pList;
}

void CreateRoomWindow::closeEvent(QCloseEvent *ev)
{
    Q_UNUSED(ev)

    deleteLater();
}

void CreateRoomWindow::keyPressEvent(QKeyEvent *ev)
{
    if ( ev->key() == Qt::Key_Return )
    {
        on_pushButton_clicked();
    }
}

CreateRoomWindow::~CreateRoomWindow()
{
    delete ui;
}

void CreateRoomWindow::on_pushButton_clicked()
{
    bool bASCII = true;

    for (int i = 0; i < ui->lineEdit->text().size(); i++)
    {
        if (ui->lineEdit->text()[i].unicode() > 127)
        {
            bASCII = false;

            break;
        }
    }

    if ( (ui->lineEdit->text().size() < 2) || (ui->lineEdit->text().size() > 20) )
    {
        QMessageBox::warning(nullptr, "Error", "A name for the room should be 2-20 characters long.");
        return;
    }


    std::vector<QString> vRoomNames = pList->getRoomNames();

    for (size_t i = 0; i < vRoomNames.size(); i++)
    {
        if (ui->lineEdit->text() == vRoomNames[i])
        {
            QMessageBox::warning(nullptr, "Error", "The room name should be unique.");
            return;
        }
    }

    if (bASCII)
    {
        size_t iMaxUsers = 0;

        if (ui->lineEdit_3->text() != "")
        {
            iMaxUsers = static_cast<size_t>(ui->lineEdit_3->text().toInt());
        }

        emit signalCreateNewRoom(ui->lineEdit->text(), ui->lineEdit_2->text(),
                                      iMaxUsers);

        close();
    }
    else
    {
        QMessageBox::warning(nullptr, "Error", "A name for the room should contain only ASCII characters.");
        return;
    }
}
