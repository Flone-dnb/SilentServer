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

    ui->label->setText("Enter a new name for the room \"" + pRoom->getRoomName() + "\":");
}



void ChangeRoomNameWindow::closeEvent(QCloseEvent *ev)
{
    Q_UNUSED(ev)

    deleteLater();
}

void ChangeRoomNameWindow::on_pushButton_clicked()
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
        emit signalRenameRoom(pRoom, ui->lineEdit->text());

        close();
    }
    else
    {
        QMessageBox::warning(nullptr, "Error", "A name for the room should contain only ASCII characters.");
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
