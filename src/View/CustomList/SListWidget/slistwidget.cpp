// This file is part of the Silent Server.
// Copyright Aleksandr "Flone" Tretyakov (github.com/Flone-dnb).
// Licensed under the ZLib license.
// Refer to the LICENSE file included.

#include "slistwidget.h"

#include <QMessageBox>

#include "View/CustomList/SListItemUser/slistitemuser.h"
#include "View/CustomList/SListItemRoom/slistitemroom.h"

SListWidget::SListWidget(QWidget *parent) : QListWidget(parent)
{
    addRoom("Welcome Room");
    addRoom("Room 1");
    addRoom("Room 2");
    addRoom("Room 3");
}

void SListWidget::addRoom(QString sRoomName, QString sPassword, size_t iMaxUsers)
{
    if (vRooms.size() == MAX_ROOMS)
    {
        QMessageBox::warning(nullptr, "Error", "Reached the maximum amount of rooms.");
    }
    else
    {
        SListItemRoom* pNewItem = new SListItemRoom(sRoomName, this, sPassword, iMaxUsers);
        pNewItem->setItemType(true);

        vRooms.push_back(pNewItem);

        addItem(pNewItem);
    }
}

SListItemUser* SListWidget::addUser(QString sUserName, SListItemRoom* pRoom)
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

void SListWidget::deleteRoom(SListItemRoom *pRoom)
{
    std::vector<SListItemUser*> vUsers = pRoom->getUsers();

    for (size_t i = 0; i < vRooms.size(); i++)
    {
        if (vRooms[i]->getRoomName() == pRoom->getRoomName())
        {
            vRooms.erase(vRooms.begin() + i);
            break;
        }
    }

    delete pRoom;

    for (size_t i = 0; i < vUsers.size(); i++)
    {
        delete vUsers[i];
    }
}

void SListWidget::deleteUser(SListItemUser *pUser)
{
    pUser->getRoom()->deleteUser(pUser);
}

void SListWidget::renameRoom(SListItemRoom *pRoom, QString sNewName)
{
    pRoom->setRoomName(sNewName);
}

void SListWidget::moveRoomUp(SListItemRoom *pRoom)
{
    // Find upper room.

    int iRow = 0;
    size_t iPosInVec = vRooms.size() - 1;

    for (size_t i = 0; i < vRooms.size(); i++)
    {
        if (vRooms[i] != pRoom)
        {
            iRow = row(vRooms[i]);
        }
        else
        {
            iPosInVec = i;
            break;
        }
    }



    // Erase room & users from list.

    takeItem( row(pRoom) );

    std::vector<SListItemUser*> vUsers = pRoom->getUsers();

    for (size_t i = 0; i < vUsers.size(); i++)
    {
        takeItem( row(vUsers[i]) );
    }


    // Insert in new place.

    insertItem(iRow, pRoom);

    for (size_t i = 0; i < vUsers.size(); i++)
    {
        insertItem(iRow + static_cast<int>(i) + 1, vUsers[i]);
    }


    // Change vRooms.

    vRooms.erase( vRooms.begin() + iPosInVec);

    vRooms.insert( vRooms.begin() + iPosInVec - 1, pRoom );
}

void SListWidget::moveRoomDown(SListItemRoom *pRoom)
{
    // Find room below.

    int iRow = 0;
    size_t iPosInVec = 1;

    for (size_t i = vRooms.size() - 1; i > 0; i--)
    {
        if (vRooms[i] != pRoom)
        {
            iRow = row(vRooms[i]);
        }
        else
        {
            iPosInVec = i;
            break;
        }
    }



    // Erase room & users from list.

    takeItem( row(pRoom) );

    std::vector<SListItemUser*> vUsers = pRoom->getUsers();

    for (size_t i = 0; i < vUsers.size(); i++)
    {
        takeItem( row(vUsers[i]) );
    }


    // Insert in new place.

    iRow -= static_cast<int>(vUsers.size());

    insertItem(iRow, pRoom);

    for (size_t i = 0; i < vUsers.size(); i++)
    {
        insertItem(iRow + static_cast<int>(i) + 1, vUsers[i]);
    }


    // Change vRooms.

    vRooms.erase( vRooms.begin() + iPosInVec);

    vRooms.insert( vRooms.begin() + iPosInVec + 1, pRoom );
}

std::vector<QString> SListWidget::getRoomNames()
{
    std::vector<QString> vNames;

    for (size_t i = 0; i < vRooms.size(); i++)
    {
        vNames.push_back(vRooms[i]->getRoomName());
    }

    return vNames;
}

size_t SListWidget::getRoomCount()
{
    return vRooms.size();
}

bool SListWidget::isAbleToCreateRoom()
{
    if (vRooms.size() == MAX_ROOMS)
    {
        QMessageBox::warning(nullptr, "Error", "Reached the maximum amount of rooms.");
        return false;
    }
    else
    {
        return true;
    }
}

SListWidget::~SListWidget()
{
}
