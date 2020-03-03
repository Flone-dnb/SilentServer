// This file is part of the Silent Server.
// Copyright Aleksandr "Flone" Tretyakov (github.com/Flone-dnb).
// Licensed under the ZLib license.
// Refer to the LICENSE file included.

#pragma once

#include <string>

class MainWindow;
class SListItemUser;
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

        void            kickUser              (SListItemUser* pListWidgetItem);
        void            changeRoomSettings    (const std::string& sOldName, const std::string& sNewName,
                                               const std::u16string& sPassword,
                                               size_t iMaxUsers);
        void            moveRoom              (const std::string& sRoomName, bool bMoveUp);


    // Archive text

        void            archiveText           (char16_t* pText, size_t iWChars);
        void            showOldText           ();


    // Settings

        void            saveNewSettings       (SettingsFile* pSettingsFile);


    // GET functions

        SettingsFile*   getSettingsFile       ();
        std::string     getServerVersion      ();
        std::string     getLastClientVersion  ();
        bool            isServerRunning       ();



    ~Controller();

private:

    ServerService*   pServerService;
    SettingsManager* pSettingsManager;
    LogManager*      pLogManager;


    bool bServerStarted;
};
