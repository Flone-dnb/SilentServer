// This file is part of the Silent Server.
// Copyright Aleksandr "Flone" Tretyakov (github.com/Flone-dnb).
// Licensed under the ZLib license.
// Refer to the LICENSE file included.

#include "slistitem.h"

SListItem::SListItem(QString sName)
{
    setText(sName);

    bIsRoom = false;
}

void SListItem::setItemType(bool bIsRoom)
{
    this->bIsRoom = bIsRoom;
}

bool SListItem::isRoom()
{
    return bIsRoom;
}

SListItem::~SListItem()
{

}
