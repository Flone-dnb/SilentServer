// This file is part of the Silent Server.
// Copyright Aleksandr "Flone" Tretyakov (github.com/Flone-dnb).
// Licensed under the ZLib license.
// Refer to the LICENSE file included.

#include "slistwidget.h"
#include "ui_slistwidget.h"

#include "View/CustomList/SListItemUser/slistitemuser.h"
#include "View/CustomList/SListItemRoom/slistitemroom.h"

SListWidget::SListWidget(QWidget *parent) :
    QListWidget(parent),
    ui(new Ui::SListWidget)
{
    ui->setupUi(this);

    addListRoom("Welcome Room");
}

void SListWidget::addListRoom(QString sRoomName)
{
    SListItemRoom* pNewItem = new SListItemRoom(sRoomName, this);
    pNewItem->setItemType(true);

    vRooms.push_back(pNewItem);

    addItem(pNewItem);
}

SListItemUser* SListWidget::addListUser(QString sUserName, SListItemRoom* pRoom)
{
    SListItemUser* pNewItem = new SListItemUser(sUserName);
    pNewItem->setItemType(false);

    if (pRoom == nullptr)
    {
        pNewItem->setRoom(vRooms[0]);
        vRooms[0]->addUser(pNewItem);
    }
    else
    {
        pNewItem->setRoom(pRoom);
        pRoom->addUser(pNewItem);
    }

    return pNewItem;
}

void SListWidget::deleteUser(SListItemUser *pUser)
{
    pUser->getRoom()->deleteUser(pUser);
}

SListWidget::~SListWidget()
{
    delete ui;
}
