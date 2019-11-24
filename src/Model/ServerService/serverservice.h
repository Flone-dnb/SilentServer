#pragma once

// Custom
#include "../src/Model/userstruct.h"

// C++
#include <vector>
#include <string>
#include <ctime>
#include <mutex>

// ============== Network ==============
// Sockets and stuff
#include <winsock2.h>
#include <ws2tcpip.h>

// Winsock 2 Library
#pragma comment(lib,"Ws2_32.lib")



class MainWindow;
class SettingsManager;
class UDPPacket;



// should be shorter than MAX_VERSION_STRING_LENGTH
#define SERVER_VERSION           "2.18.0"
#define CLIENT_SUPPORTED_VERSION "2.21.0"


// ------------------------------------------------------------------------------------------------
// ------------------------------------------------------------------------------------------------
// ------------------------------------------------------------------------------------------------



class ServerService
{

public:

    ServerService(MainWindow* pMainWindow, SettingsManager* pSettingsManager);




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


    // Closing

        void  sendFINtoSocket             (SOCKET socketToClose);
        void  responseToFIN               (UserStruct* userToClose, bool bUserLost = false);
        void  shutdownAllUsers            ();


    // GET functions

        std::string    getServerVersion          ();
        std::string    getLastClientVersion      ();
        unsigned short getPingNormalBelow        ();
        unsigned short getPingWarningBelow       ();

private:

    void catchUDPPackets();
    void eraseUDPPacket();


    // --------------------------------


    MainWindow*              pMainWindow;
    SettingsManager*         pSettingsManager;


    // Users
    SOCKET                   listenTCPSocket;
    SOCKET                   UDPsocket;
    SOCKET                   udpPingCheckSocket;
    std::vector<UserStruct*> users;
    std::vector<UDPPacket*>  vUDPPackets;


    // Ping
    unsigned short           iPingNormalBelow;
    unsigned short           iPingWarningBelow;


    std::mutex               mtxUDPPackets;


    std::string              serverVersion;
    std::string              clientLastSupportedVersion;


    int                      iUsersConnectedCount;
    int                      iUsersConnectedToVOIP;


    bool                     bWinSockStarted;
    bool                     bTextListen;
    bool                     bVoiceListen;
};
