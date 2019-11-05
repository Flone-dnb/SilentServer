#pragma once


#include <QMainWindow>


class QCloseEvent;

namespace Ui
{
    class SettingsWindow;
}



class SettingsWindow : public QMainWindow
{
    Q_OBJECT

signals:

    void signalApply (unsigned short iServerPort);

public:

    explicit SettingsWindow(QWidget *parent = nullptr);



    void setSettings (QString sServerPort);



    ~SettingsWindow();

protected:

    void closeEvent (QCloseEvent* ev);

private slots:

    void on_pushButton_clicked();

private:

    Ui::SettingsWindow *ui;
};
