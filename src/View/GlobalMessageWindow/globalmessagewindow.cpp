// This file is part of the Silent Server.
// Copyright Aleksandr "Flone" Tretyakov (github.com/Flone-dnb).
// Licensed under the ZLib license.
// Refer to the LICENSE file included.

#include "globalmessagewindow.h"
#include "ui_globalmessagewindow.h"

#include <QKeyEvent>
#include <QMessageBox>

GlobalMessageWindow::GlobalMessageWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::GlobalMessageWindow)
{
    ui->setupUi(this);


    // Hide maximize & minimize buttons

    Qt::WindowFlags flags = windowFlags();
    flags &= ~Qt::WindowMaximizeButtonHint;
    flags &= ~Qt::WindowMinimizeButtonHint;
    setWindowFlags(flags);
}

void GlobalMessageWindow::on_pushButton_clicked()
{
    if ( ui->plainTextEdit->toPlainText().size() > 1200 )
    {
        QMessageBox::warning(this, "Warning", "Your text is too big!");
    }
    else
    {
        emit signalServerMessage( ui->plainTextEdit->toPlainText() );
        close();
    }
}

GlobalMessageWindow::~GlobalMessageWindow()
{
    delete ui;
}

void GlobalMessageWindow::keyPressEvent(QKeyEvent *event)
{
    if ( event->key() == Qt::Key_Return )
    {
        on_pushButton_clicked();
    }
}

void GlobalMessageWindow::closeEvent(QCloseEvent *ev)
{
    Q_UNUSED(ev)

    deleteLater();
}
