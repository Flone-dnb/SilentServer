// This file is part of the Silent Server.
// Copyright Aleksandr "Flone" Tretyakov (github.com/Flone-dnb).
// Licensed under the ZLib license.
// Refer to the LICENSE file included.

#include "controller.h"



// Custom
#include "Model/ServerService/serverservice.h"
#include "Model/SettingsManager/settingsmanager.h"
#include "Model/LogManager/logmanager.h"
#include "Model/SettingsManager/SettingsFile.h"


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

void Controller::kickUser(SListItemUser *pListWidgetItem)
{
    pServerService ->kickUser(pListWidgetItem);
}

void Controller::changeRoomSettings(const std::string &sOldName, const std::string &sNewName, const std::u16string& sPassword,
                                    size_t iMaxUsers)
{
    TODO;
}

void Controller::createRoom(const std::string &sName, const std::u16string &sPassword, size_t iMaxUsers)
{
    pServerService->createRoom(sName, sPassword, iMaxUsers);
}

void Controller::deleteRoom(const std::string &sName)
{
    pServerService->deleteRoom(sName);
}

void Controller::moveRoom(const std::string &sRoomName, bool bMoveUp)
{
    pServerService->moveRoom(sRoomName, bMoveUp);
}

void Controller::archiveText(char16_t *pText, size_t iWChars)
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
    if (bServerStarted == false
        && pSettingsManager->getCurrentSettings()->iPort != pSettingsFile->iPort)
    {
        // If changed port then delete old temp files
        // because they have port in the names.
        pLogManager->eraseTempFile();
    }

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
