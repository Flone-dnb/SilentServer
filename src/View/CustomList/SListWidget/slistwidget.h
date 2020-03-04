// This file is part of the Silent Server.
// Copyright Aleksandr "Flone" Tretyakov (github.com/Flone-dnb).
// Licensed under the ZLib license.
// Refer to the LICENSE file included.

#pragma once

// Qt
#include <QListWidget>

// STL
#include <vector>

class SListItemRoom;
class SListItemUser;


#define MAX_ROOMS 100


class SListWidget : public QListWidget
{
    Q_OBJECT

public:
    explicit SListWidget       (QWidget *parent = nullptr);


    void           addRoom     (QString sRoomName, QString sPassword = "", size_t iMaxUsers = 0);
    SListItemUser* addUser     (QString sUserName, SListItemRoom* pRoom = nullptr);
    void           deleteRoom  (SListItemRoom* pRoom);
    void           deleteUser  (SListItemUser* pUser);

    void           renameRoom  (SListItemRoom* pRoom, QString sNewName);

    void           moveRoomUp  (SListItemRoom* pRoom);
    void           moveRoomDown(SListItemRoom* pRoom);

    std::vector<QString> getRoomNames();
    size_t               getRoomCount();
    bool                 isAbleToCreateRoom();


    ~SListWidget();

private:

    std::vector<SListItemRoom*> vRooms;
};
