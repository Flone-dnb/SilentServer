// This file is part of the Silent Server.
// Copyright Aleksandr "Flone" Tretyakov (github.com/Flone-dnb).
// Licensed under the ZLib license.
// Refer to the LICENSE file included.

#pragma once

// Qt
#include <QMainWindow>
#include <QTimer>

// STL
#include <thread>
#include <vector>
#include <string>
#include <mutex>



class QMouseEvent;
class Controller;
class QListWidgetItem;
class QCloseEvent;
class QHideEvent;
class QSystemTrayIcon;
class SettingsFile;
class QMenu;
class QAction;

namespace Ui {
class MainWindow;
}

#if _WIN32
#define S16String S16String
#define S16Char   S16Char
#elif __linux__
#define S16String std::u16string
#define S16Char   char16_t
#endif


#define ARCHIVE_HALF_TEXT_SLIDER_VALUE 100


// ------------------------------------------------------------------------------------------------
// ------------------------------------------------------------------------------------------------
// ------------------------------------------------------------------------------------------------


class MainWindow : public QMainWindow
{
    Q_OBJECT


public:

    explicit MainWindow(QWidget *parent = nullptr);

    ~MainWindow();



    // Updating / Printing / Showing

        void             showMessageBox             (bool bErrorBox,         const S16String& sMessage, bool bEmitSignal = false);
        void             printOutput                (std::string errorText,  bool bEmitSignal = false);
        void             updateOnlineUsersCount     (int iNewAmount);
        void             showOldText                (S16Char* pText);
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
        void             signalShowOldText           (S16Char* pText);
        void             signalClearChatWindow       ();

protected:

    void                 hideEvent                   (QHideEvent* event);
    void                 closeEvent                  (QCloseEvent *event);

private slots:


    // This-to-This slots

        void             slotShowMessageBox          (bool bErrorBox,              const QString& sMessage);
        void             slotSetPingToUser           (QListWidgetItem* pListItem,  int ping);
        void             typeSomeOnOutputLog         (QString text);
        void             slotShowOldText             (S16Char* pText);
        void             slotClearChatWindow         ();


    // Tray Icon slot

        void             slotTrayIconActivated       ();


    // Settings Window

        void             slotApplyNewSettings        (SettingsFile* pSettingsFile);


    // Menu

        void             on_actionStart_triggered    ();
        void             on_actionAbout_2_triggered  ();
        void             on_actionSettings_triggered ();


    // Slider on output log

        void             slotSliderMoved(int iValue);


    // Context Menu

        void on_listWidget_users_customContextMenuRequested(const QPoint &pos);
        void kickUser();

private:


    void checkTextSize();

    // --------------------------------------


    Ui::MainWindow*  ui;
    Controller*      pController;


    QMenu*           pMenuContextMenu;
    QAction*         pActionKick;


    QSystemTrayIcon* pTrayIcon;


    std::mutex       mtxPrintOutput;
    std::mutex       mtxListUsers;


    bool             bInternalTextWork;
};
