// This file is part of the Silent Server.
// Copyright Aleksandr "Flone" Tretyakov (github.com/Flone-dnb).
// Licensed under the ZLib license.
// Refer to the LICENSE file included.

#pragma once

#include "View/CustomList/SListItem/slistitem.h"

class SListItemRoom;


class SListItemUser : public SListItem
{
public:

    SListItemUser(QString sName);


    void setRoom(SListItemRoom* pRoom);
    void setPing(int iPing);

    SListItemRoom* getRoom();


    ~SListItemUser() override;

private:

    SListItemRoom* pRoom;

    QString sName;
};
