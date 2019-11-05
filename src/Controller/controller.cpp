#include "controller.h"

// Custom
#include "../src/Model/ServerService/serverservice.h"

Controller::Controller(MainWindow* pMainWindow)
{
    pServerService = new ServerService(pMainWindow);
    bServerStarted = false;
}

std::string Controller::getServerVersion()
{
    return pServerService->getServerVersion();
}

std::string Controller::getLastClientVersion()
{
    return pServerService->getLastClientVersion();
}

unsigned short Controller::getPingNormalBelow()
{
    return pServerService->getPingNormalBelow();
}

unsigned short Controller::getPingWarningBelow()
{
    return pServerService->getPingWarningBelow();
}

bool Controller::isServerRunning()
{
    return bServerStarted;
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

void Controller::saveNewSettings(unsigned short iServerPort)
{
    pServerService ->saveNewSettings(iServerPort);
}

void Controller::openSettings()
{
    pServerService ->showSettings();
}

Controller::~Controller()
{
    delete pServerService;
}
