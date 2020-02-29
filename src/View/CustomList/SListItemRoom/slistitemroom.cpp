// This file is part of the Silent Server.
// Copyright Aleksandr "Flone" Tretyakov (github.com/Flone-dnb).
// Licensed under the ZLib license.
// Refer to the LICENSE file included.

#include "slistitemroom.h"

#include "View/CustomList/SListWidget/slistwidget.h"
#include "View/CustomList/SListItemUser/slistitemuser.h"


SListItemRoom::SListItemRoom(QString sName, SListWidget* pList)
{
    this->pList = pList;

    setText(sName);
}

void SListItemRoom::addUser(SListItemUser *pUser)
{
    vUsers.push_back(pUser);


    int iStartRow = pList->row(this);

    int iInsertRowIndex = -1;

    for (int i = iStartRow + 1; i < pList->count(); i++)
    {
        if (dynamic_cast<SListItem*>(pList->item(i))->isRoom())
        {
            // Found next room.
            iInsertRowIndex = i;
            break;
        }
    }


    if (iInsertRowIndex == -1)
    {
        // Next room wasn't found.
        pList->addItem(pUser);
    }
    else
    {
        pList->insertItem(iInsertRowIndex, pUser);
    }
}

void SListItemRoom::deleteUser(SListItemUser *pUser)
{
    delete pUser;

    for (size_t i = 0; i < vUsers.size(); i++)
    {
        if (vUsers[i] == pUser)
        {
            vUsers.erase( vUsers.begin() + i );
        }
    }
}

SListItemRoom::~SListItemRoom()
{

}
