// This file is part of the Silent Server.
// Copyright Aleksandr "Flone" Tretyakov (github.com/Flone-dnb).
// Licensed under the ZLib license.
// Refer to the LICENSE file included.

#pragma once

#include <QListWidgetItem>

class SListItem : public QListWidgetItem
{
public:

    SListItem() = default;
    SListItem         (QString sName);



    void setItemType  (bool bIsRoom);

    bool isRoom       ();



    virtual ~SListItem();

private:

    bool bIsRoom;
};
