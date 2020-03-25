// This file is part of the Silent Server.
// Copyright Aleksandr "Flone" Tretyakov (github.com/Flone-dnb).
// Licensed under the ZLib license.
// Refer to the LICENSE file included.

#pragma once
#include <QMainWindow>

namespace Ui {
class GlobalMessageWindow;
}

class GlobalMessageWindow : public QMainWindow
{
    Q_OBJECT

signals:
    void signalServerMessage(QString sMessage);

public:
    explicit GlobalMessageWindow(QWidget *parent = nullptr);
    ~GlobalMessageWindow() override;

protected:
    void keyPressEvent (QKeyEvent* event) override;
    void closeEvent(QCloseEvent* ev) override;

private slots:
    void on_pushButton_clicked();

private:
    Ui::GlobalMessageWindow *ui;
};
