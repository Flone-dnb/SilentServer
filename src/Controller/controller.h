#pragma once

#include <string>

class MainWindow;
class ServerService;


class Controller
{

public:

    Controller(MainWindow* pMainWindow);



    // Start / Stop

        void            start                 ();
        void            stop                  ();


    // Settings

        void            saveNewSettings       (unsigned short iServerPort);
        void            openSettings          ();


    // GET functions

        std::string     getServerVersion      ();
        std::string     getLastClientVersion  ();
        unsigned short  getPingNormalBelow    ();
        unsigned short  getPingWarningBelow   ();
        bool            isServerRunning       ();



    ~Controller();

private:

    ServerService* pServerService;

    bool bServerStarted;
};
