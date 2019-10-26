#pragma once

// Qt
#include <QMainWindow>


class QCloseEvent;
namespace Ui
{
    class AboutWindow;
}


// --------------------------------------------------------------------------------------------------------------------
// --------------------------------------------------------------------------------------------------------------------
// --------------------------------------------------------------------------------------------------------------------


class AboutWindow : public QMainWindow
{
    Q_OBJECT

public:

    explicit AboutWindow(QString sSilentVersion, QString sSilentLastClientVersion, QWidget *parent = nullptr);





    ~AboutWindow();

protected:

    void closeEvent (QCloseEvent* pEvent);

private slots:
    void on_pushButton_clicked();

private:

    Ui::AboutWindow *ui;
};
