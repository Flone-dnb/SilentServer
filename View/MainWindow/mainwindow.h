#pragma once

// Qt
#include <QMainWindow>
#include <QTimer>

// C++
#include <thread>
#include <vector>
#include <string>

// ============== Network ==============
// Sockets and stuff
#include <winsock2.h>

// Adress translation
#include <ws2tcpip.h>

// Winsock 2 Library
#pragma comment(lib,"Ws2_32.lib")



class QMouseEvent;
class UserStruct;
class Controller;
class QListWidgetItem;

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

    void closeEvent(QCloseEvent *event);

private slots:

    void on_actionAbout_triggered();

    void slotSetPingToUser(QListWidgetItem* pListItem, int ping);

    void slotClearChatWindow();

    void on_actionStart_triggered();

    void typeSomeOnOutputLog(QString text);

private:

    Ui::MainWindow* ui;
    Controller*     pController;

    bool bAlreadyClosing;
};
