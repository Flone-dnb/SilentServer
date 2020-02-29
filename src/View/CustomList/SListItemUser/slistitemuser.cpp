// This file is part of the Silent Server.
// Copyright Aleksandr "Flone" Tretyakov (github.com/Flone-dnb).
// Licensed under the ZLib license.
// Refer to the LICENSE file included.

#include "slistitemuser.h"

SListItemUser::SListItemUser(QString sName)
{
    QString sFormatedName = "    ";
    sFormatedName += sName;

    setText(sFormatedName);

    pRoom = nullptr;

    this->sName = sName;
}

void SListItemUser::setRoom(SListItemRoom *pRoom)
{
    this->pRoom = pRoom;
}

void SListItemUser::setPing(int iPing)
{
    QString sFormatedName = "    ";
    sFormatedName += this->sName;
    sFormatedName += " [" + QString::number(iPing) + " ms]";

    setText(sFormatedName);
}

SListItemRoom *SListItemUser::getRoom()
{
    return pRoom;
}

SListItemUser::~SListItemUser()
{
}
