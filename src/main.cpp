// This file is part of the Silent Server.
// Copyright Aleksandr "Flone" Tretyakov (github.com/Flone-dnb).
// Licensed under the ZLib license.
// Refer to the LICENSE file included.

#include "../src/View/MainWindow/mainwindow.h"
#include <QApplication>

int main(int argc, char *argv[])
{
    // Enable run-time memory check for debug builds.
#if defined(DEBUG) || defined(_DEBUG)
    _CrtSetDbgFlag( _CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF );
#endif


    QApplication a(argc, argv);
    MainWindow w;
    w.show();

    return a.exec();
}
