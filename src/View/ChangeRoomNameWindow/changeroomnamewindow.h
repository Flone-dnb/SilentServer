// This file is part of the Silent Server.
// Copyright Aleksandr "Flone" Tretyakov (github.com/Flone-dnb).
// Licensed under the ZLib license.
// Refer to the LICENSE file included.

#pragma once


#include <QMainWindow>
#include <QCloseEvent>

namespace Ui
{
    class ChangeRoomNameWindow;
}

class SListItemRoom;
class SListWidget;

class ChangeRoomNameWindow : public QMainWindow
{
    Q_OBJECT

signals:

    void signalChangeRoomSettings(SListItemRoom* pRoom, QString sName, QString sPassword, size_t iMaxUsers);

public:

    explicit ChangeRoomNameWindow(SListItemRoom* pRoom, SListWidget* pList, QWidget *parent = nullptr);

    ~ChangeRoomNameWindow() override;

protected:

    void keyPressEvent (QKeyEvent* event) override;
    void closeEvent(QCloseEvent* ev) override;

private slots:

    void on_pushButton_clicked();

private:

    Ui::ChangeRoomNameWindow *ui;


    SListWidget*   pList;
    SListItemRoom* pRoom;
};
