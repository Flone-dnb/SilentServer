#pragma once

// Custom
#include "../src/Model/userstruct.h"

// C++
#include <vector>
#include <string>
#include <ctime>

// ============== Network ==============
// Sockets and stuff
#include <winsock2.h>
#include <ws2tcpip.h>

// Winsock 2 Library
#pragma comment(lib,"Ws2_32.lib")



class MainWindow;


class ServerService
{

public:

    ServerService(MainWindow* pMainWindow);




    // Starting

        bool  startWinSock                ();
        void  startToListenForConnection  ();
        void  prepareForVoiceConnection   ();


    // Listen

        void  listenForMessage            (UserStruct* userToListen);
        void  listenForVoiceMessage       (UserStruct* userToListen);
        void  processMessage              (UserStruct* userToListen);
        void  listenForNewTCPConnections  ();


    // Ping

        void  checkPing                   ();
        void  sendPingToAll               ();


    // Settings

        void  saveNewSettings             (unsigned short iServerPort);
        void  showSettings                ();


    // Closing

        void  sendFINtoSocket             (SOCKET socketToClose);
        void  responseToFIN               (UserStruct* userToClose, bool bUserLost = false);
        void  shutdownAllUsers            ();


    // GET functions

        std::string    getServerVersion          ();
        std::string    getLastClientVersion      ();
        unsigned short getServerPortFromSettings ();

private:


    MainWindow*              pMainWindow;


    // Users
    SOCKET                   listenTCPSocket;
    SOCKET                   UDPsocket;
    SOCKET                   udpPingCheckSocket;
    std::vector<UserStruct*> users;


    std::string              serverVersion;
    std::string              clientLastSupportedVersion;
    std::wstring             sSettingsFileName;


    int                      iUsersConnectedCount;
    int                      iUsersConnectedToVOIP;


    bool                     bWinSockStarted;
    bool                     bTextListen;
    bool                     bVoiceListen;
};
