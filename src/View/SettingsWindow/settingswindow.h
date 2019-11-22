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

    explicit SettingsWindow(SettingsFile* pSettingsFile, QWidget *parent = nullptr);



    void setSettings (SettingsFile* pSettingsFile);



    ~SettingsWindow();

protected:

    void closeEvent (QCloseEvent* ev);

private slots:

    void on_pushButton_clicked();

private:

    Ui::SettingsWindow *ui;
};
