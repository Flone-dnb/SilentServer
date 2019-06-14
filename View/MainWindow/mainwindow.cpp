#include "mainwindow.h"
#include "ui_mainwindow.h"

// Custom
#include "Controller/controller.h"

// Qt
#include <QMouseEvent>
#include <QMessageBox>

// C++
#include <string>


MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    bAlreadyClosing = false;

    pController = new Controller(this);

    connect(this, &MainWindow::signalTypeOnOutput,    this, &MainWindow::typeSomeOnOutputLog);
    connect(this, &MainWindow::signalClearChatWindow, this, &MainWindow::slotClearChatWindow);
}





void MainWindow::printOutput(std::string errorText, bool bEmitSignal)
{
    if (bEmitSignal == false)
    {
      ui->plainTextEdit->appendPlainText( QString::fromStdString(errorText) );
    }
    else
    {
        // This function (printOutput) was called from another thread (not main thread)
        // so if we will append text to 'plaintTextEdit' crash can occur because you
        // cannot change GDI from another thread (it's something with how Windows and GDI works with threads)
        // Because of that we will emit signal to main thread to append text.
        // Right? I dunno "it just works". :p
        emit signalTypeOnOutput(QString::fromStdString(errorText));
    }
}

void MainWindow::updateOnlineUsersCount(int iNewAmount)
{
    ui->label_2->setText("Connected: " + QString::number(iNewAmount));
}

QListWidgetItem* MainWindow::addNewUserToList(std::string userName)
{
    ui->listWidget->addItem( QString::fromStdString(userName) );
    return ui->listWidget->item( ui->listWidget->model()->rowCount()-1 );
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
    ui->plainTextEdit->appendPlainText(text);
}

void MainWindow::on_actionAbout_triggered()
{
    QMessageBox::about(nullptr,"FChat","FChat Server. Version: " + QString::fromStdString(pController->getServerVersion()) + ".");
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
