#pragma once

// Qt
#include <QMainWindow>
#include <QTimer>

// STL
#include <thread>
#include <vector>
#include <string>
#include <mutex>

// ============== Network ==============
// Sockets and stuff
#include <winsock2.h>

// Adress translation
#include <ws2tcpip.h>

// Winsock 2 Library
#pragma comment(lib,"Ws2_32.lib")



class QMouseEvent;
class Controller;
class QListWidgetItem;
class QCloseEvent;
class QHideEvent;
class QSystemTrayIcon;

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:

    explicit MainWindow(QWidget *parent = nullptr);

    ~MainWindow();

    // ServerService will call them:
        void printOutput(std::string errorText, bool bEmitSignal = false);
        void updateOnlineUsersCount(int iNewAmount);
        QListWidgetItem* addNewUserToList(std::string userName);
        void setPingToUser(QListWidgetItem* pListItem, int ping);
        void deleteUserFromList(QListWidgetItem* pListItem);
        void changeStartStopActionText(bool bStop);
        void clearChatWindow();

signals:

    void signalTypeOnOutput(QString text);
    void signalSetPingToUser(QListWidgetItem* pListItem, int ping);
    void signalClearChatWindow();

protected:

    void hideEvent(QHideEvent* event);
    void closeEvent(QCloseEvent *event);

private slots:

    void on_actionAbout_triggered();
    void slotSetPingToUser(QListWidgetItem* pListItem, int ping);
    void slotClearChatWindow();
    void on_actionStart_triggered();
    void typeSomeOnOutputLog(QString text);
    void slotTrayIconActivated();

private:

    Ui::MainWindow*  ui;
    Controller*      pController;


    QSystemTrayIcon* pTrayIcon;


    std::mutex       mtxPrintOutput;


    bool             bAlreadyClosing;
};
