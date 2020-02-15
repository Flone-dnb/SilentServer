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

#if _WIN32
#define S16String std::wstring
#define S16Char   S16Char
#elif __linux__
#define S16String std::u16string
#define S16Char   char16_t
#endif


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

        void            archiveText           (S16Char* pText, size_t iWChars);
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
