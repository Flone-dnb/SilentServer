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

// Custom
#include "../src/Controller/controller.h"
#include "../src/View/AboutWindow/aboutwindow.h"
#include "../src/View/SettingsWindow/settingswindow.h"
#include "../src/View/PingColorParams.h"
#include "../src/Model/SettingsManager/SettingsFile.h"


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



    pController = new Controller(this);




    // Setup context menu for 'connected users' list

    ui ->listWidget_users ->setContextMenuPolicy (Qt::CustomContextMenu);
    ui ->listWidget_users ->setViewMode          (QListView::ListMode);

    pMenuContextMenu    = new QMenu(this);

    pMenuContextMenu ->setStyleSheet("QMenuBar { background-color: transparent; color: white; } QMenuBar::item { background-color: transparent; } QMenuBar::item::selected { background-color: qlineargradient(spread:pad, x1:0.5, y1:0, x2:0.5, y2:1, stop:0 rgba(156, 11, 11, 255), stop:1 rgba(168, 0, 0, 255)); } QMenu { background-color: qlineargradient(spread:pad, x1:0.5, y1:0, x2:0.5, y2:1, stop:0 rgba(26, 26, 26, 100), stop:0.605809 rgba(19, 19, 19, 255), stop:1 rgba(26, 26, 26, 100)); color: white; } QMenu::item::selected { background-color: qlineargradient(spread:pad, x1:0.5, y1:0, x2:0.5, y2:1, stop:0 rgba(156, 11, 11, 255), stop:1 rgba(168, 0, 0, 255)); } QMenu::separator { background-color: rgb(50, 0, 0); height: 2px; margin-left: 10px; margin-right: 5px; }");

    pActionKick         = new QAction("Kick");

    pMenuContextMenu ->addAction(pActionKick);

    connect(pActionKick, &QAction::triggered, this, &MainWindow::kickUser);



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
void MainWindow::showMessageBox(bool bErrorBox, const std::wstring &sMessage, bool bEmitSignal)
{
    if (bEmitSignal)
    {
        emit signalShowMessageBox( bErrorBox, QString::fromStdWString(sMessage) );
    }
    else
    {
        if (bErrorBox)
        {
            QMessageBox::warning( this, "Error", QString::fromStdWString( sMessage ) );
        }
        else
        {
            QMessageBox::information( this, "Information", QString::fromStdWString( sMessage ) );
        }
    }
}

void MainWindow::updateOnlineUsersCount(int iNewAmount)
{
    ui ->label_2 ->setText("Connected: " + QString::number(iNewAmount));
}

QListWidgetItem* MainWindow::addNewUserToList(std::string userName)
{
    userName += " [-- ms]";

    ui ->listWidget_users        ->addItem ( QString::fromStdString(userName) );
    return ui ->listWidget_users ->item    ( ui ->listWidget_users ->model() ->rowCount()-1 );
}

void MainWindow::setPingToUser(QListWidgetItem* pListItem, int ping)
{
    emit signalSetPingToUser(pListItem, ping);
}

void MainWindow::deleteUserFromList(QListWidgetItem* pListItem)
{
    mtxListUsers .lock();

    delete pListItem;

    mtxListUsers .unlock();
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

void MainWindow::showOldText(wchar_t *pText)
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

void MainWindow::slotShowOldText(wchar_t *pText)
{
    mtxPrintOutput .lock();


    std::wstring sText(pText);

    delete[] pText;


    QString sNewText = "";
    sNewText += QString::fromStdWString(sText);

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

void MainWindow::slotSetPingToUser(QListWidgetItem* pListItem, int ping)
{
    mtxListUsers .lock();

    if (pListItem != nullptr)
    {
        QString userNameWithOldPing = pListItem->text();

        QString userNameWithNewPing = "";
        for (int i = 0; i < userNameWithOldPing.size(); i++)
        {
            if (userNameWithOldPing[i] != " ")
            {
                userNameWithNewPing += userNameWithOldPing[i];
            }
            else break;
        }

        if (ping != 0)
        {
            userNameWithNewPing += " [" + QString::number(ping) + " ms]";
        }
        else
        {
            userNameWithNewPing += " [-- ms]";
        }

        pListItem->setText(userNameWithNewPing);



        // Set color

        if      (ping <= pController ->getPingNormalBelow())
        {
            pListItem ->setForeground( QColor(PING_NORMAL_R, PING_NORMAL_G, PING_NORMAL_B) );
        }
        else if (ping <= pController ->getPingWarningBelow())
        {
            pListItem ->setForeground( QColor(PING_WARNING_R, PING_WARNING_G, PING_WARNING_B) );
        }
        else
        {
            pListItem ->setForeground( QColor(PING_BAD_R, PING_BAD_G, PING_BAD_B) );
        }
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
    QListWidgetItem* pItem = ui ->listWidget_users ->itemAt(pos);

    if (pItem)
    {
        QPoint globalPos = ui ->listWidget_users ->mapToGlobal(pos);

        pMenuContextMenu ->exec(globalPos);
    }
}

void MainWindow::kickUser()
{
    if ( ui ->listWidget_users ->currentRow() >= 0 )
    {
        pController ->kickUser( ui ->listWidget_users ->currentItem() );
    }
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


        std::wstring sOldWString = sText .left( sText .size() / 2 ) .toStdWString();

        size_t iOldTextSizeInWChars = sOldWString .size() * 2;

        wchar_t* pOldText = new wchar_t[ iOldTextSizeInWChars + 1 ];
        memset(pOldText, 0, (iOldTextSizeInWChars * sizeof(wchar_t)) + sizeof(wchar_t));


        std::memcpy(pOldText, sOldWString .c_str(), (iOldTextSizeInWChars * sizeof(wchar_t)));

        pController ->archiveText(pOldText, iOldTextSizeInWChars);
    }
}

MainWindow::~MainWindow()
{
    delete pController;

    delete ui;
}
