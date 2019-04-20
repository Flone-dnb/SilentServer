#include "controller.h"

// Custom
#include "Model/ServerService/serverservice.h"

Controller::Controller(MainWindow* pMainWindow)
{
    pServerService = new ServerService(pMainWindow);
}

void Controller::start()
{
    pServerService->startWinSock();
}

void Controller::stop()
{
    pServerService->shutdownAllUsers();
}

Controller::~Controller()
{
    delete pServerService;
}
