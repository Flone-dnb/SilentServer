#include "controller.h"



// Custom
#include "Model/ServerService/serverservice.h"
#include "Model/SettingsManager/settingsmanager.h"
#include "Model/LogManager/logmanager.h"


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
        else
        {
            pLogManager ->eraseTempFile();

            bServerStarted = pServerService->startWinSock();
        }

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

void Controller::kickUser(QListWidgetItem *pListWidgetItem)
{
    pServerService ->kickUser(pListWidgetItem);
}

void Controller::archiveText(wchar_t *pText, size_t iWChars)
{
    if (pLogManager)
    {
        pLogManager ->archiveText(pText, iWChars);
    }
    else
    {
        delete[] pText;
    }
}

void Controller::showOldText()
{
    if (pLogManager)
    {
        pLogManager ->showOldText();
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
