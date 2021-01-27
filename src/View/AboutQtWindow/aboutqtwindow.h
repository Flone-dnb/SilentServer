// This file is part of the Silent Server.
// Copyright Aleksandr "Flone" Tretyakov (github.com/Flone-dnb).
// Licensed under the ZLib license.
// Refer to the LICENSE file included.

#pragma once

#include <QMainWindow>

namespace Ui {
class AboutQtWindow;
}

class AboutQtWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit AboutQtWindow(QWidget *parent = nullptr);
    ~AboutQtWindow();

protected:
    void closeEvent(QCloseEvent* event);

private:
    Ui::AboutQtWindow *ui;
};
