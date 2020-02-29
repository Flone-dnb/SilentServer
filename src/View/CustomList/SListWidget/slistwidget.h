// This file is part of the Silent Server.
// Copyright Aleksandr "Flone" Tretyakov (github.com/Flone-dnb).
// Licensed under the ZLib license.
// Refer to the LICENSE file included.

#pragma once

// Qt
#include <QListWidget>

// STL
#include <vector>

namespace Ui
{
    class SListWidget;
}


class SListItemRoom;
class SListItemUser;


class SListWidget : public QListWidget
{
    Q_OBJECT

public:
    explicit SListWidget(QWidget *parent = nullptr);

    void addListRoom(QString sRoomName);
    SListItemUser* addListUser(QString sUserName, SListItemRoom* pRoom = nullptr);

    void deleteUser(SListItemUser* pUser);

    ~SListWidget();

private:

    Ui::SListWidget *ui;


    std::vector<SListItemRoom*> vRooms;
};
