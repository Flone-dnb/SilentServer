// This file is part of the Silent Server.
// Copyright Aleksandr "Flone" Tretyakov (github.com/Flone-dnb).
// Licensed under the ZLib license.
// Refer to the LICENSE file included.

#pragma once

#include <string>

class MainWindow;
class QListWidgetItem;
class ServerService;
class SettingsManager;
class SettingsFile;
class LogManager;


class Controller
{

public:

    Controller(MainWindow* pMainWindow);



    // Start / Stop

        bool            start                 ();
        void            stop                  ();


    // Commands

        void            kickUser              (QListWidgetItem* pListWidgetItem);


    // Archive text

        void            archiveText           (wchar_t* pText, size_t iWChars);
        void            showOldText           ();


    // Settings

        void            saveNewSettings       (SettingsFile* pSettingsFile);


    // GET functions

        SettingsFile*   getSettingsFile       ();
        std::string     getServerVersion      ();
        std::string     getLastClientVersion  ();
        unsigned short  getPingNormalBelow    ();
        unsigned short  getPingWarningBelow   ();
        bool            isServerRunning       ();



    ~Controller();

private:

    ServerService*   pServerService;
    SettingsManager* pSettingsManager;
    LogManager*      pLogManager;


    bool bServerStarted;
};
