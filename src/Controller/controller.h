#pragma once

#include <string>

class MainWindow;
class ServerService;
class SettingsManager;
class SettingsFile;


class Controller
{

public:

    Controller(MainWindow* pMainWindow);



    // Start / Stop

        bool            start                 ();
        void            stop                  ();


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

    bool bServerStarted;
};
