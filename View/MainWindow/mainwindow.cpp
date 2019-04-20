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

    connect(this,&MainWindow::signalTypeOnOutput,this,&MainWindow::typeSomeOnOutputLog);
}

MainWindow::~MainWindow()
{
    delete pController;

    delete ui;
}

void MainWindow::printOutput(std::string errorText, bool bEmitSignal)
{
    if (bEmitSignal == false)
    {
      ui->plainTextEdit->appendPlainText(QString::fromStdString(errorText));
    }
    else
    {
        // This function (printOutput) was called from another thread (not main thread)
        // so if we will append text to 'plaintTextEdit' we will have errors because you
        // cannot append info from another thread.
        // Because of that we will emit signal to main thread to append text
        // Right? I dunno "it just works". :p
        emit signalTypeOnOutput(QString::fromStdString(errorText));
    }
}

void MainWindow::updateOnlineUsersCount(int iNewAmount)
{
    ui->label_2->setText("Main lobby: "+QString::number(iNewAmount));
}

QListWidgetItem* MainWindow::addNewUserToList(std::string userName)
{
    ui->listWidget->addItem(QString::fromStdString(userName));
    return ui->listWidget->item(ui->listWidget->model()->rowCount()-1);
}

void MainWindow::deleteUserFromList(QListWidgetItem *pListItem)
{
    delete pListItem;
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    if (bAlreadyClosing == false)
    {
        // First time pressed exit button
        ui->plainTextEdit->appendPlainText("\nShutting down...");

        pController->stop();

        ui->label_2->setText("Main lobby: 0");

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
    QMessageBox::about(nullptr,"FChat","FChat (server). Version: 1.0 (12.04.2019)");
}

void MainWindow::on_actionStart_triggered()
{
    // Start Winsock2

    ui->plainTextEdit->clear();
    ui->plainTextEdit->appendPlainText("Starting...");

    pController->start();
}
