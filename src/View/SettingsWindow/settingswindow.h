// This file is part of the Silent Server.
// Copyright Aleksandr "Flone" Tretyakov (github.com/Flone-dnb).
// Licensed under the ZLib license.
// Refer to the LICENSE file included.

#pragma once


// Qt
#include <QMainWindow>


class QCloseEvent;
class SettingsFile;

namespace Ui
{
    class SettingsWindow;
}



class SettingsWindow : public QMainWindow
{
    Q_OBJECT

signals:

    void signalApply (SettingsFile* pSettingsFile);

public:

    explicit SettingsWindow(QWidget *parent = nullptr);



    void setSettings (SettingsFile* pSettingsFile);



    ~SettingsWindow();

protected:

    void keyPressEvent (QKeyEvent* event);
    void closeEvent    (QCloseEvent* ev);

private slots:

    void on_pushButton_clicked();

    void on_pushButton_log_file_select_path_clicked();

    void on_checkBox_log_stateChanged(int arg1);

private:

    Ui::SettingsWindow *ui;
};
