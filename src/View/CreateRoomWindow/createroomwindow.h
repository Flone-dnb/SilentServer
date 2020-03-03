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

class SListWidget;

class CreateRoomWindow : public QMainWindow
{
    Q_OBJECT

signals:
    void signalCreateNewRoom(QString sName, QString sPassword, size_t iMaxUsers);
public:
    explicit CreateRoomWindow(SListWidget* pList, QWidget *parent = nullptr);
    ~CreateRoomWindow();
protected:
    void closeEvent(QCloseEvent* ev);
    void keyPressEvent(QKeyEvent* ev);
private slots:
    void on_pushButton_clicked();

private:
    Ui::CreateRoomWindow *ui;
    SListWidget* pList;
};
