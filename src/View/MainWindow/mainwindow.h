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



    // Updating / Printing / Showing

        void             showMessageBox             (bool bErrorBox,         const std::wstring& sMessage, bool bEmitSignal = false);
        void             printOutput                (std::string errorText,  bool bEmitSignal = false);
        void             showSettingsWindow         (unsigned short iServerPort);
        void             updateOnlineUsersCount     (int iNewAmount);
        void             clearChatWindow            ();


    // Ping

        void             setPingToUser              (QListWidgetItem* pListItem, int ping);


    // Users

        QListWidgetItem* addNewUserToList           (std::string userName);
        void             deleteUserFromList         (QListWidgetItem* pListItem);


    // Menu

        void             changeStartStopActionText  (bool bStop);


signals:


    // This-to-This signals

        void             signalShowMessageBox        (bool bErrorBox,              const QString& sMessage);
        void             signalSetPingToUser         (QListWidgetItem* pListItem,  int ping);
        void             signalTypeOnOutput          (QString text);
        void             signalClearChatWindow       ();

protected:

    void                 hideEvent                   (QHideEvent* event);
    void                 closeEvent                  (QCloseEvent *event);

private slots:


    // This-to-This slots

        void             slotShowMessageBox          (bool bErrorBox,              const QString& sMessage);
        void             slotSetPingToUser           (QListWidgetItem* pListItem,  int ping);
        void             typeSomeOnOutputLog         (QString text);
        void             slotClearChatWindow         ();


    // Tray Icon slot

        void             slotTrayIconActivated       ();


    // Settings Window

        void             slotApplyNewSettings        (unsigned short iServerPort);


    // Menu

        void             on_actionStart_triggered    ();
        void             on_actionAbout_2_triggered  ();
        void             on_actionSettings_triggered ();

private:

    Ui::MainWindow*  ui;
    Controller*      pController;


    QSystemTrayIcon* pTrayIcon;


    std::mutex       mtxPrintOutput;


    bool             bAlreadyClosing;
};
