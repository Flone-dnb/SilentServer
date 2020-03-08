// This file is part of the Silent Server.
// Copyright Aleksandr "Flone" Tretyakov (github.com/Flone-dnb).
// Licensed under the ZLib license.
// Refer to the LICENSE file included.

#include "changeroomnamewindow.h"
#include "ui_changeroomnamewindow.h"

#include <QMessageBox>
#include <QKeyEvent>

#include "View/CustomList/SListItemRoom/slistitemroom.h"
#include "View/CustomList/SListWidget/slistwidget.h"

ChangeRoomNameWindow::ChangeRoomNameWindow(SListItemRoom* pRoom, SListWidget* pList, QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::ChangeRoomNameWindow)
{
    ui->setupUi(this);


    // Hide maximize & minimize buttons

    Qt::WindowFlags flags = windowFlags();
    flags &= ~Qt::WindowMaximizeButtonHint;
    flags &= ~Qt::WindowMinimizeButtonHint;
    setWindowFlags(flags);


    this->pRoom = pRoom;
    this->pList = pList;

    if (pList->row(pRoom) == 0)
    {
        ui ->lineEdit_name ->setText(pRoom ->getRoomName());

        ui ->lineEdit_password ->setEnabled(false);
        ui ->lineEdit_count ->setEnabled(false);
    }
    else
    {
        ui ->lineEdit_name ->setText(pRoom ->getRoomName());
        ui ->lineEdit_password ->setText(pRoom ->getPassword());

        if (pRoom ->getMaxUsers() != 0)
        {
            ui ->lineEdit_count ->setText(QString::number(pRoom ->getMaxUsers()));
        }
        else
        {
            ui ->lineEdit_count ->setText("");
        }
    }
}



void ChangeRoomNameWindow::closeEvent(QCloseEvent *ev)
{
    Q_UNUSED(ev)

    deleteLater();
}

void ChangeRoomNameWindow::on_pushButton_clicked()
{
    bool bASCII = true;

    if (pRoom->getRoomName() != ui->lineEdit_name->text())
    {
        for (int i = 0; i < ui->lineEdit_name->text().size(); i++)
        {
            if (ui->lineEdit_name->text()[i].unicode() > 127)
            {
                bASCII = false;

                break;
            }
        }

        if ( (ui->lineEdit_name->text().size() < 2) || (ui->lineEdit_name->text().size() > 20) )
        {
            QMessageBox::warning(nullptr, "Error", "A name for the room should be 2-20 characters long.");
            return;
        }


        std::vector<QString> vRoomNames = pList->getRoomNames();

        for (size_t i = 0; i < vRoomNames.size(); i++)
        {
            if (ui->lineEdit_name->text() == vRoomNames[i])
            {
                QMessageBox::warning(nullptr, "Error", "The room name should be unique.");
                return;
            }
        }
    }

    if (bASCII)
    {
        if ( (pRoom->getUsers().size() > ui->lineEdit_count->text().toUInt())
             && ui->lineEdit_count->text().toUInt() != 0)
        {
            QMessageBox::warning(nullptr, "Error", "The max amount of users is lower than there are users right now in this room.");
        }
        else
        {
            emit signalChangeRoomSettings(pRoom, ui->lineEdit_name->text(), ui->lineEdit_password->text(),
                                          static_cast<size_t>(ui->lineEdit_count->text().toInt()));

            close();
        }
    }
    else
    {
        QMessageBox::warning(nullptr, "Error", "A name for the room should contain only ASCII characters.");
        return;
    }
}

void ChangeRoomNameWindow::keyPressEvent(QKeyEvent *event)
{
    if ( event->key() == Qt::Key_Return )
    {
        on_pushButton_clicked();
    }
}

ChangeRoomNameWindow::~ChangeRoomNameWindow()
{
    delete ui;
}
