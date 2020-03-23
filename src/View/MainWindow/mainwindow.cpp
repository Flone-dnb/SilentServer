// This file is part of the Silent Server.
// Copyright Aleksandr "Flone" Tretyakov (github.com/Flone-dnb).
// Licensed under the ZLib license.
// Refer to the LICENSE file included.

#include "mainwindow.h"
#include "ui_mainwindow.h"

// Qt
#include <QMouseEvent>
#include <QMessageBox>
#include <QCloseEvent>
#include <QHideEvent>
#include <QSystemTrayIcon>
#include <QMenu>
#include <QAction>
#include <QListWidgetItem>
#include <QScrollBar>

#if _WIN32
using std::memcpy;
#endif

// Custom
#include "Controller/controller.h"
#include "View/AboutWindow/aboutwindow.h"
#include "View/SettingsWindow/settingswindow.h"
#include "View/PingColorParams.h"
#include "Model/SettingsManager/SettingsFile.h"
#include "View/CustomList/SListItemUser/slistitemuser.h"
#include "View/CustomList/SListItemRoom/slistitemroom.h"
#include "View/ChangeRoomNameWindow/changeroomnamewindow.h"
#include "View/CreateRoomWindow/createroomwindow.h"


MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    // Tray icon
    pTrayIcon      = new QSystemTrayIcon(this);
    QIcon icon     = QIcon(":/appMainIcon.png");
    pTrayIcon->setIcon(icon);
    connect(pTrayIcon, &QSystemTrayIcon::activated, this, &MainWindow::slotTrayIconActivated);



    qRegisterMetaType<QTextCursor>("QTextCursor");
    qRegisterMetaType<QVector<int>>("QVector<int>");



    pController = new Controller(this);




    // Setup context menu for 'connected users' list

    ui ->listWidget_users ->setContextMenuPolicy (Qt::CustomContextMenu);
    ui ->listWidget_users ->setViewMode          (QListView::ListMode);




    // Create menu when clicked on empty space.

    pMenuEmpty = new QMenu(this);
    connect(pMenuEmpty, &QMenu::aboutToHide, this, &MainWindow::slotMenuClose);

    pMenuEmpty ->setStyleSheet("QMenuBar { background-color: transparent; color: white; } QMenuBar::item { background-color: transparent; } QMenuBar::item::selected { background-color: qlineargradient(spread:pad, x1:0.5, y1:0, x2:0.5, y2:1, stop:0 rgba(156, 11, 11, 255), stop:1 rgba(168, 0, 0, 255)); } QMenu { background-color: qlineargradient(spread:pad, x1:0.5, y1:0, x2:0.5, y2:1, stop:0 rgba(26, 26, 26, 100), stop:0.605809 rgba(19, 19, 19, 255), stop:1 rgba(26, 26, 26, 100)); color: white; } QMenu::item::selected { background-color: qlineargradient(spread:pad, x1:0.5, y1:0, x2:0.5, y2:1, stop:0 rgba(156, 11, 11, 255), stop:1 rgba(168, 0, 0, 255)); } QMenu::separator { background-color: rgb(50, 0, 0); height: 2px; margin-left: 10px; margin-right: 5px; }");

    pActionCreateRoom = new QAction("Create Room");

    pMenuEmpty ->addAction(pActionCreateRoom);

    connect(pActionCreateRoom, &QAction::triggered, this, &MainWindow::createRoom);




    // Create menu for user.

    pMenuUser   = new QMenu(this);
    connect(pMenuUser, &QMenu::aboutToHide, this, &MainWindow::slotMenuClose);

    pMenuUser ->setStyleSheet("QMenuBar { background-color: transparent; color: white; } QMenuBar::item { background-color: transparent; } QMenuBar::item::selected { background-color: qlineargradient(spread:pad, x1:0.5, y1:0, x2:0.5, y2:1, stop:0 rgba(156, 11, 11, 255), stop:1 rgba(168, 0, 0, 255)); } QMenu { background-color: qlineargradient(spread:pad, x1:0.5, y1:0, x2:0.5, y2:1, stop:0 rgba(26, 26, 26, 100), stop:0.605809 rgba(19, 19, 19, 255), stop:1 rgba(26, 26, 26, 100)); color: white; } QMenu::item::selected { background-color: qlineargradient(spread:pad, x1:0.5, y1:0, x2:0.5, y2:1, stop:0 rgba(156, 11, 11, 255), stop:1 rgba(168, 0, 0, 255)); } QMenu::separator { background-color: rgb(50, 0, 0); height: 2px; margin-left: 10px; margin-right: 5px; }");

    pActionKick = new QAction("Kick");

    pMenuUser ->addAction(pActionKick);
    pMenuUser ->addSeparator();
    pMenuUser ->addAction(pActionCreateRoom);

    connect(pActionKick, &QAction::triggered, this, &MainWindow::kickUser);



    // Create menu for room.

    pMenuRoom   = new QMenu(this);
    connect(pMenuRoom, &QMenu::aboutToHide, this, &MainWindow::slotMenuClose);

    pMenuRoom ->setStyleSheet("QMenuBar { background-color: transparent; color: white; } QMenuBar::item { background-color: transparent; } QMenuBar::item::selected { background-color: qlineargradient(spread:pad, x1:0.5, y1:0, x2:0.5, y2:1, stop:0 rgba(156, 11, 11, 255), stop:1 rgba(168, 0, 0, 255)); } QMenu { background-color: qlineargradient(spread:pad, x1:0.5, y1:0, x2:0.5, y2:1, stop:0 rgba(26, 26, 26, 100), stop:0.605809 rgba(19, 19, 19, 255), stop:1 rgba(26, 26, 26, 100)); color: white; } QMenu::item::selected { background-color: qlineargradient(spread:pad, x1:0.5, y1:0, x2:0.5, y2:1, stop:0 rgba(156, 11, 11, 255), stop:1 rgba(168, 0, 0, 255)); } QMenu::separator { background-color: rgb(50, 0, 0); height: 2px; margin-left: 10px; margin-right: 5px; }");

    pActionChangeSettings = new QAction("Change Room Settings");
    pActionMoveRoomUp     = new QAction("Move Up");
    pActionMoveRoomDown   = new QAction("Move Down");
    pActionDeleteRoom     = new QAction("Delete Room");

    pMenuRoom ->addAction(pActionChangeSettings);
    pMenuRoom ->addAction(pActionMoveRoomUp);
    pMenuRoom ->addAction(pActionMoveRoomDown);
    pMenuRoom ->addAction(pActionCreateRoom);
    pMenuRoom ->addAction(pActionDeleteRoom);

    connect(pActionChangeSettings, &QAction::triggered, this, &MainWindow::changeRoomSettings);
    connect(pActionMoveRoomUp,     &QAction::triggered, this, &MainWindow::moveRoomUp);
    connect(pActionMoveRoomDown,   &QAction::triggered, this, &MainWindow::moveRoomDown);
    connect(pActionDeleteRoom,     &QAction::triggered, this, &MainWindow::deleteRoom);



    // Slider
    connect(ui ->plainTextEdit ->verticalScrollBar(), &QScrollBar::valueChanged, this, &MainWindow::slotSliderMoved);

    bInternalTextWork = false;



    // This to This
    connect(this, &MainWindow::signalTypeOnOutput,    this, &MainWindow::typeSomeOnOutputLog);
    connect(this, &MainWindow::signalClearChatWindow, this, &MainWindow::slotClearChatWindow);
    connect(this, &MainWindow::signalSetPingToUser,   this, &MainWindow::slotSetPingToUser);
    connect(this, &MainWindow::signalShowMessageBox,  this, &MainWindow::slotShowMessageBox);
    connect(this, &MainWindow::signalShowOldText,     this, &MainWindow::slotShowOldText);
}





void MainWindow::printOutput(std::string errorText, bool bEmitSignal)
{
    if (bEmitSignal == false)
    {
        mtxPrintOutput .lock   ();

        ui ->plainTextEdit ->appendPlainText( QString::fromStdString(errorText) );

        mtxPrintOutput .unlock ();

        checkTextSize();
    }
    else
    {
        emit signalTypeOnOutput(QString::fromStdString(errorText));
    }
}
void MainWindow::showMessageBox(bool bErrorBox, const std::u16string &sMessage, bool bEmitSignal)
{
    if (bEmitSignal)
    {
        emit signalShowMessageBox( bErrorBox, QString::fromStdU16String(sMessage) );
    }
    else
    {
        if (bErrorBox)
        {
            QMessageBox::warning( this, "Error", QString::fromStdU16String( sMessage ) );
        }
        else
        {
            QMessageBox::information( this, "Information", QString::fromStdU16String( sMessage ) );
        }
    }
}

void MainWindow::updateOnlineUsersCount(int iNewAmount)
{
    ui ->label_2 ->setText("Connected: " + QString::number(iNewAmount));
}

SListItemUser* MainWindow::addNewUserToList(std::string userName)
{
    return ui->listWidget_users->addUser(QString::fromStdString(userName));

    //ui ->listWidget_users        ->addItem ( QString::fromStdString(userName) );
    //return ui ->listWidget_users ->item    ( ui ->listWidget_users ->model() ->rowCount()-1 );
}

void MainWindow::setPingToUser(SListItemUser* pListItem, int ping)
{
    emit signalSetPingToUser(pListItem, ping);
}

void MainWindow::deleteUserFromList(SListItemUser* pUser)
{
    mtxListUsers .lock();

    ui ->listWidget_users->deleteUser(pUser);
    //delete pListItem;

    mtxListUsers .unlock();
}

std::vector<std::string> MainWindow::getRoomNames()
{
    std::vector<std::string> vRoomNames;

    std::vector<QString> vRooms = ui ->listWidget_users ->getRoomNames();

    for (size_t i = 0; i < vRooms.size(); i++)
    {
        vRoomNames.push_back(vRooms[i].toStdString());
    }

    return vRoomNames;
}

std::vector<std::string> MainWindow::getUsersOfRoomIndex(size_t i)
{
    std::vector<std::string> vUsers;

    std::vector<SListItemUser*> vUserNames = ui ->listWidget_users ->getRoom(i) ->getUsers();

    for (size_t i = 0; i < vUserNames.size(); i++)
    {
        vUsers.push_back(vUserNames[i]->getName().toStdString());
    }

    return vUsers;
}

std::u16string MainWindow::getRoomPassword(size_t iRoomIndex)
{
    return ui ->listWidget_users ->getRoom(iRoomIndex) ->getPassword() .toStdU16String();
}

std::u16string MainWindow::getRoomPassword(std::string sRoomName)
{
    return ui ->listWidget_users ->getRoomPassword(QString::fromStdString(sRoomName)).toStdU16String();
}

unsigned short MainWindow::getRoomMaxUsers(size_t iRoomIndex)
{
    return static_cast<unsigned short>(ui ->listWidget_users ->getRoom(iRoomIndex) ->getMaxUsers());
}

void MainWindow::moveUserToRoom(SListItemUser *pUser, std::string sRoomName)
{
    ui ->listWidget_users ->moveUser(pUser, QString::fromStdString(sRoomName));
}

bool MainWindow::checkRoomSettings(std::string sRoomName, bool* pbPasswordNeeded, bool* pbRoomFull)
{
    std::vector<QString> vRooms = ui ->listWidget_users ->getRoomNames();

    QString sRoomToEnter = QString::fromStdString(sRoomName);

    bool bRoomFound = false;

    SListItemRoom* pRoom = nullptr;

    for (size_t i = 0; i < vRooms.size(); i++)
    {
        if (sRoomToEnter == vRooms[i])
        {
            bRoomFound = true;
            pRoom = ui ->listWidget_users ->getRoom(i);
            break;
        }
    }

    if (bRoomFound == false)
    {
        return true;
    }

    if ( (pRoom->getUsersCount() == pRoom->getMaxUsers()) && (pRoom->getMaxUsers() != 0) )
    {
        *pbRoomFull = true;
    }

    if (pRoom->getPassword() != "")
    {
        *pbPasswordNeeded = true;
    }

    return false;
}

void MainWindow::changeStartStopActionText(bool bStop)
{
    if (bStop)
    {
        ui ->actionStart ->setText("Stop Server");
    }
    else
    {
        ui ->actionStart ->setText("Start Server");
    }
}

void MainWindow::clearChatWindow()
{
    emit signalClearChatWindow();
}

void MainWindow::showOldText(char16_t *pText)
{
    emit signalShowOldText(pText);
}

void MainWindow::hideEvent(QHideEvent *event)
{
    Q_UNUSED(event)

    hide();
    pTrayIcon ->show();
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    if (pController ->isServerRunning())
    {
        pController ->stop();
        event       ->ignore();

        ui ->plainTextEdit ->appendPlainText("Press Exit again to close.");
    }
}

void MainWindow::slotShowMessageBox(bool bErrorBox, const QString &sMessage)
{
    if (bErrorBox)
    {
        QMessageBox::warning( this, "Error", sMessage );
    }
    else
    {
        QMessageBox::information( this, "Information", sMessage );
    }
}

void MainWindow::typeSomeOnOutputLog(QString text)
{
    mtxPrintOutput .lock   ();

    ui ->plainTextEdit ->appendPlainText (text);

    mtxPrintOutput .unlock ();

    checkTextSize();
}

void MainWindow::slotShowOldText(char16_t *pText)
{
    mtxPrintOutput .lock();


    std::u16string sText(pText);

    delete[] pText;


    QString sNewText = "";
    sNewText += QString::fromStdU16String(sText);

    sNewText += ui ->plainTextEdit ->toPlainText() .right( ui ->plainTextEdit ->toPlainText() .size() - 10 ); // 10: ".........."


    bInternalTextWork = true;

    ui ->plainTextEdit ->clear();

    ui ->plainTextEdit ->appendPlainText(sNewText);

    bInternalTextWork = false;


    mtxPrintOutput .unlock();
}

void MainWindow::slotTrayIconActivated()
{
    pTrayIcon ->hide();
    raise();
    activateWindow();
    showNormal();
}

void MainWindow::slotApplyNewSettings(SettingsFile* pSettingsFile)
{
    pController ->saveNewSettings(pSettingsFile);
}

void MainWindow::slotSetPingToUser(SListItemUser* pListItem, int ping)
{
    mtxListUsers .lock();

    if (pListItem != nullptr)
    {
//        QString userNameWithOldPing = pListItem->text();

//        QString userNameWithNewPing = "";
//        for (int i = 0; i < userNameWithOldPing.size(); i++)
//        {
//            if (userNameWithOldPing[i] != ' ')
//            {
//                userNameWithNewPing += userNameWithOldPing[i];
//            }
//            else break;
//        }

//        if (ping != 0)
//        {
//            userNameWithNewPing += " [" + QString::number(ping) + " ms]";
//        }
//        else
//        {
//            userNameWithNewPing += " [-- ms]";
//        }

//        pListItem->setText(userNameWithNewPing);

        pListItem->setPing(ping);
    }

    mtxListUsers .unlock();
}

void MainWindow::slotClearChatWindow()
{
    mtxPrintOutput .lock();

    ui->plainTextEdit->clear();

    mtxPrintOutput .unlock();
}

void MainWindow::on_actionStart_triggered()
{
    if ( pController ->start() )
    {
        ui ->plainTextEdit ->appendPlainText("Could not start the server.");
    }
}

void MainWindow::on_actionAbout_2_triggered()
{
    AboutWindow* pAboutWindow = new AboutWindow (
                                                    QString::fromStdString(pController->getServerVersion()),
                                                    QString::fromStdString(pController->getLastClientVersion()),
                                                    this
                                                );
    pAboutWindow ->setWindowModality (Qt::WindowModality::WindowModal);
    pAboutWindow ->show();
}


void MainWindow::on_actionSettings_triggered()
{
    SettingsWindow* pSettingsWindow = new SettingsWindow (this);
    pSettingsWindow ->setWindowModality( Qt::WindowModality::WindowModal );

    connect(pSettingsWindow, &SettingsWindow::signalApply, this, &MainWindow::slotApplyNewSettings);

    pSettingsWindow ->setSettings(pController ->getSettingsFile());

    pSettingsWindow ->show();
}

void MainWindow::slotSliderMoved(int iValue)
{
    if ( bInternalTextWork == false )
    {
        if ( iValue == 0 )
        {
            pController ->showOldText();
        }
    }
}


void MainWindow::on_listWidget_users_customContextMenuRequested(const QPoint &pos)
{
    SListItem* pItem = dynamic_cast<SListItem*>(ui ->listWidget_users ->itemAt(pos));

    if (pItem)
    {
        QPoint globalPos = ui ->listWidget_users ->mapToGlobal(pos);

        if (pItem->isRoom())
        {
            SListItemRoom* pRoomItem = dynamic_cast<SListItemRoom*>(pItem);

            if (pRoomItem ->getRoomName() == ui ->listWidget_users ->getRoomNames()[0])
            {
                pActionDeleteRoom ->setVisible(false);
            }
            else
            {
                pActionDeleteRoom ->setVisible(true);
            }

            if (ui ->listWidget_users ->getRoomCount() > 1)
            {
                if ( (pRoomItem->getRoomName() == ui ->listWidget_users ->getRoomNames()[1])
                     || (pRoomItem->getRoomName() == ui ->listWidget_users ->getRoomNames()[0]))
                {
                    pActionMoveRoomUp ->setVisible(false);
                }
                else
                {
                    pActionMoveRoomUp ->setVisible(true);
                }

                if ( (pRoomItem->getRoomName() ==
                                     ui ->listWidget_users ->getRoomNames()[ ui ->listWidget_users ->getRoomNames().size() - 1 ])
                     || (pRoomItem->getRoomName() == ui ->listWidget_users ->getRoomNames()[0]))
                {
                    pActionMoveRoomDown ->setVisible(false);
                }
                else
                {
                    pActionMoveRoomDown ->setVisible(true);
                }
            }
            else
            {
                pActionMoveRoomUp ->setVisible(false);
                pActionMoveRoomDown ->setVisible(false);
            }


            pMenuRoom ->exec(globalPos);
        }
        else
        {
            pMenuUser ->exec(globalPos);
        }
    }
    else
    {
        QPoint globalPos = ui ->listWidget_users ->mapToGlobal(pos);

        pMenuEmpty ->exec(globalPos);
    }
}

void MainWindow::kickUser()
{
    if ( ui ->listWidget_users ->currentRow() >= 0 )
    {
        pController ->kickUser( dynamic_cast<SListItemUser*>(ui ->listWidget_users ->currentItem()) );

        ui ->listWidget_users ->clearSelection();
    }
}

void MainWindow::changeRoomSettings()
{
    if ( ui ->listWidget_users ->currentRow() >= 0 )
    {
        ChangeRoomNameWindow* pWindow =
                new ChangeRoomNameWindow(dynamic_cast<SListItemRoom*>(ui ->listWidget_users ->currentItem()),
                                         ui->listWidget_users,
                                         this);

        pWindow->setWindowModality(Qt::WindowModality::WindowModal);

        connect(pWindow, &ChangeRoomNameWindow::signalChangeRoomSettings, this, &MainWindow::slotChangeRoomSettings);

        pWindow->show();

        ui ->listWidget_users ->clearSelection();
    }
}

void MainWindow::moveRoomUp()
{
    if ( ui ->listWidget_users ->currentRow() >= 0 )
    {
        pController ->moveRoom( dynamic_cast<SListItemRoom*>(ui ->listWidget_users ->currentItem())->getRoomName().toStdString(),
                                true );

        ui ->listWidget_users ->moveRoomUp( dynamic_cast<SListItemRoom*>(ui ->listWidget_users ->currentItem()) );

        ui ->listWidget_users ->clearSelection();
    }
}

void MainWindow::moveRoomDown()
{
    if ( ui ->listWidget_users ->currentRow() >= 0 )
    {
        pController ->moveRoom( dynamic_cast<SListItemRoom*>(ui ->listWidget_users ->currentItem())->getRoomName().toStdString(),
                                false );

        ui ->listWidget_users ->moveRoomDown( dynamic_cast<SListItemRoom*>(ui ->listWidget_users ->currentItem()) );

        ui ->listWidget_users ->clearSelection();
    }
}

void MainWindow::createRoom()
{
    if (ui ->listWidget_users ->isAbleToCreateRoom())
    {
        CreateRoomWindow* pWindow = new CreateRoomWindow(ui->listWidget_users, this);

        connect(pWindow, &CreateRoomWindow::signalCreateNewRoom, this, &MainWindow::slotCreateNewRoom);

        pWindow->setWindowModality(Qt::WindowModality::WindowModal);

        pWindow->show();
    }
}

void MainWindow::deleteRoom()
{
    if ( ui ->listWidget_users ->currentRow() >= 0 )
    {
        SListItemRoom* pRoom = dynamic_cast<SListItemRoom*>(ui ->listWidget_users ->currentItem());

        if (pRoom->getUsersCount() > 0)
        {
            QMessageBox::warning(nullptr, "Error", "Cannot delete the room because there are users in it.");
        }
        else
        {
            pController->deleteRoom(pRoom->getRoomName().toStdString());

            ui ->listWidget_users ->deleteRoom(pRoom);
        }
    }
}

void MainWindow::slotChangeRoomSettings(SListItemRoom *pRoom,  QString sName, QString sPassword, size_t iMaxUsers)
{
    QString sOldName = pRoom->getRoomName();

    pRoom->setRoomName(sName);
    pRoom->setRoomPassword(sPassword);
    pRoom->setRoomMaxUsers(iMaxUsers);

    pController->changeRoomSettings(sOldName.toStdString(), sName.toStdString(), pRoom->getPassword().toStdU16String(),
                                    pRoom->getMaxUsers());
}

void MainWindow::slotCreateNewRoom(QString sName, QString sPassword, size_t iMaxUsers)
{
    ui->listWidget_users->addRoom(sName, sPassword, iMaxUsers);
}

void MainWindow::slotMenuClose()
{
    ui ->listWidget_users ->clearSelection();
}

void MainWindow::checkTextSize()
{
    if ( ui ->plainTextEdit ->verticalScrollBar() ->value() >= ARCHIVE_HALF_TEXT_SLIDER_VALUE )
    {
        mtxPrintOutput .lock();


        QString sText = ui ->plainTextEdit ->toPlainText();

        QString sNewText = "..........";
        sNewText += sText .right( sText .size() / 2 );


        bInternalTextWork = true;

        ui->plainTextEdit->clear();

        ui ->plainTextEdit ->appendPlainText (sNewText);


        bInternalTextWork = false;

        mtxPrintOutput .unlock();

        std::u16string sOldWString = sText .left( sText .size() / 2 ) .toStdU16String();

        size_t iOldTextSizeInWChars = sOldWString .size() * 2;

        char16_t* pOldText = new char16_t[ iOldTextSizeInWChars + 1 ];
        memset(pOldText, 0, (iOldTextSizeInWChars * sizeof(char16_t)) + sizeof(char16_t));


        memcpy(pOldText, sOldWString .c_str(), (iOldTextSizeInWChars * sizeof(char16_t)));

        pController ->archiveText(pOldText, iOldTextSizeInWChars);
    }
}

MainWindow::~MainWindow()
{
    delete pMenuRoom;
    delete pMenuUser;
    delete pMenuEmpty;

    delete pController;

    delete ui;
}
