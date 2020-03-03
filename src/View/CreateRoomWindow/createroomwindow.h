// This file is part of the Silent Server.
// Copyright Aleksandr "Flone" Tretyakov (github.com/Flone-dnb).
// Licensed under the ZLib license.
// Refer to the LICENSE file included.

#pragma once

#include <QMainWindow>

namespace Ui
{
    class CreateRoomWindow;
}

class CreateRoomWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit CreateRoomWindow(QWidget *parent = nullptr);
    ~CreateRoomWindow();

private:
    Ui::CreateRoomWindow *ui;
};
