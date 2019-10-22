#include "mainwindow.h"
#include "ui_mainwindow.h"

// Custom
#include "../src/Controller/controller.h"

// Qt
#include <QMouseEvent>
#include <QMessageBox>


MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    bAlreadyClosing = false;

    pController = new Controller(this);

    connect(this, &MainWindow::signalTypeOnOutput,    this, &MainWindow::typeSomeOnOutputLog);
    connect(this, &MainWindow::signalClearChatWindow, this, &MainWindow::slotClearChatWindow);
    connect(this, &MainWindow::signalSetPingToUser,   this, &MainWindow::slotSetPingToUser);
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

        ui->label_2->setText("Connected: 0");

        event->ignore();
        bAlreadyClosing = true;

        ui->plainTextEdit->appendPlainText("Press Exit again to close.");
    }
}

void MainWindow::typeSomeOnOutputLog(QString text)
{
    mtxPrintOutput .lock   ();

    ui ->plainTextEdit ->appendPlainText (text);

    mtxPrintOutput .unlock ();
}

void MainWindow::on_actionAbout_triggered()
{
    QMessageBox::about(this, "FChat", "FChat Server. Version: " + QString::fromStdString(pController->getServerVersion()) + "."
                                      "\nLast supported client version: " + QString::fromStdString(pController->getLastClientVersion()) + "."
                                      "\n\nCopyright (c) 2019 Aleksandr \"Flone\" Tretyakov (github.com/Flone-dnb).");
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
    }
}

void MainWindow::slotClearChatWindow()
{
    ui->plainTextEdit->clear();
}

void MainWindow::on_actionStart_triggered()
{
    // Start Winsock2

    pController->start();
}




MainWindow::~MainWindow()
{
    delete pController;

    delete ui;
}
