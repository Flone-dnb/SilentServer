#include "mainwindow.h"
#include "ui_mainwindow.h"

// Qt
#include <QMouseEvent>
#include <QMessageBox>
#include <QCloseEvent>
#include <QHideEvent>
#include <QSystemTrayIcon>

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


    bAlreadyClosing = false;



    pController = new Controller(this);


    // This to This
    connect(this, &MainWindow::signalTypeOnOutput,    this, &MainWindow::typeSomeOnOutputLog);
    connect(this, &MainWindow::signalClearChatWindow, this, &MainWindow::slotClearChatWindow);
    connect(this, &MainWindow::signalSetPingToUser,   this, &MainWindow::slotSetPingToUser);
    connect(this, &MainWindow::signalShowMessageBox,  this, &MainWindow::slotShowMessageBox);
}





void MainWindow::printOutput(std::string errorText, bool bEmitSignal)
{
    if (bEmitSignal == false)
    {
        mtxPrintOutput .lock   ();

        ui ->plainTextEdit ->appendPlainText( QString::fromStdString(errorText) );

        mtxPrintOutput .unlock ();
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
    ui->label_2->setText("Connected: " + QString::number(iNewAmount));
}

QListWidgetItem* MainWindow::addNewUserToList(std::string userName)
{
    userName += " [-- ms]";

    ui->listWidget->addItem( QString::fromStdString(userName) );
    return ui->listWidget->item( ui->listWidget->model()->rowCount()-1 );
}

void MainWindow::setPingToUser(QListWidgetItem *pListItem, int ping)
{
    emit signalSetPingToUser(pListItem, ping);
}

void MainWindow::deleteUserFromList(QListWidgetItem *pListItem)
{
    delete pListItem;
}

void MainWindow::changeStartStopActionText(bool bStop)
{
    if (bStop)
    {
        ui->actionStart->setText("Stop Server");
    }
    else
    {
        ui->actionStart->setText("Start Server");
    }
}

void MainWindow::clearChatWindow()
{
    emit signalClearChatWindow();
}

void MainWindow::hideEvent(QHideEvent *event)
{
    Q_UNUSED(event)

    hide();
    pTrayIcon->show();
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    if (pController->isServerRunning() == false)
    {
        return;
    }

    if (bAlreadyClosing == false)
    {
        // First time pressed exit button
        pController->stop();

        event->ignore();
        bAlreadyClosing = true;

        ui->plainTextEdit->appendPlainText("Press Exit again to close.");
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
}

void MainWindow::slotTrayIconActivated()
{
    pTrayIcon->hide();
    raise();
    activateWindow();
    showNormal();
}

void MainWindow::slotApplyNewSettings(SettingsFile* pSettingsFile)
{
    pController ->saveNewSettings(pSettingsFile);
}

void MainWindow::slotSetPingToUser(QListWidgetItem *pListItem, int ping)
{
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

        if      (ping <= pController->getPingNormalBelow())
        {
            pListItem->setForeground(QColor(PING_NORMAL_R, PING_NORMAL_G, PING_NORMAL_B));
        }
        else if (ping <= pController->getPingWarningBelow())
        {
            pListItem->setForeground(QColor(PING_WARNING_R, PING_WARNING_G, PING_WARNING_B));
        }
        else
        {
            pListItem->setForeground(QColor(PING_BAD_R, PING_BAD_G, PING_BAD_B));
        }
    }
}

void MainWindow::slotClearChatWindow()
{
    ui->plainTextEdit->clear();
}

void MainWindow::on_actionStart_triggered()
{
    if ( pController->start() )
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



MainWindow::~MainWindow()
{
    delete pController;

    delete ui;
}
