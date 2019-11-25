#include "controller.h"



// Custom
#include "../src/Model/ServerService/serverservice.h"
#include "../src/Model/SettingsManager/settingsmanager.h"
#include "../src/Model/LogManager/logmanager.h"


Controller::Controller(MainWindow* pMainWindow)
{
    pSettingsManager = new SettingsManager(pMainWindow);

    if (pSettingsManager ->getCurrentSettings())
    {
        pLogManager      = new LogManager(pMainWindow, pSettingsManager);
        pServerService   = new ServerService(pMainWindow, pSettingsManager, pLogManager);
    }
    else
    {
        pLogManager    = nullptr;
        pServerService = nullptr;
    }


    bServerStarted   = false;
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

bool Controller::start()
{
    if (pSettingsManager ->getCurrentSettings())
    {
        if (bServerStarted) stop();
        else bServerStarted = pServerService->startWinSock();

        return false;
    }
    else
    {
        return true;
    }
}

void Controller::stop()
{
    if (bServerStarted)
    {
        pServerService ->shutdownAllUsers();

        pLogManager ->stop();

        bServerStarted = false;
    }
}

void Controller::saveNewSettings(SettingsFile* pSettingsFile)
{
    pSettingsManager ->saveSettings(pSettingsFile);
}

SettingsFile *Controller::getSettingsFile()
{
    return pSettingsManager ->getCurrentSettings();
}

Controller::~Controller()
{
    if (pLogManager)
    {
        delete pLogManager;
    }

    if (pServerService)
    {
        delete pServerService;
    }

    delete pSettingsManager;
}
