// This file is part of the Silent Server.
// Copyright Aleksandr "Flone" Tretyakov (github.com/Flone-dnb).
// Licensed under the ZLib license.
// Refer to the LICENSE file included.

#pragma once


#include <vector>

#include "View/CustomList/SListItem/slistitem.h"


class SListItemUser;
class SListWidget;


class SListItemRoom : public SListItem
{
public:

    SListItemRoom(QString sName, SListWidget* pList);

    void addUser(SListItemUser* pUser);
    void deleteUser(SListItemUser* pUser);


    void    setRoomName (QString sName);

    std::vector<SListItemUser*> getUsers();
    QString getRoomName ();



    ~SListItemRoom() override;

private:

    std::vector<SListItemUser*> vUsers;

    SListWidget* pList;

    QString sRoomName;
};
