#include "controller.h"

// Custom
#include "Model/ServerService/serverservice.h"

Controller::Controller(MainWindow* pMainWindow)
{
    pServerService = new ServerService(pMainWindow);
    bServerStarted = false;
}

std::string Controller::getServerVersion()
{
    return pServerService->getServerVersion();
}

void Controller::start()
{
    if (bServerStarted) stop();
    else bServerStarted = pServerService->startWinSock();
}

void Controller::stop()
{
    if (bServerStarted)
    {
        pServerService->shutdownAllUsers();
        bServerStarted = false;
    }
}

Controller::~Controller()
{
    delete pServerService;
}
