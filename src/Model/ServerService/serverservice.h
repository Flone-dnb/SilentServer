#pragma once


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

// Custom
#include "Model/userstruct.h"



class MainWindow;
class SettingsManager;
class LogManager;
class UDPPacket;



// should be shorter than MAX_VERSION_STRING_LENGTH
#define SERVER_VERSION           "2.20.1"
#define CLIENT_SUPPORTED_VERSION "2.23.1"



// ------------------------------------------------------------------------------------------------
// ------------------------------------------------------------------------------------------------
// ------------------------------------------------------------------------------------------------



class ServerService
{

public:

    ServerService(MainWindow* pMainWindow, SettingsManager* pSettingsManager, LogManager* pLogManager);




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

        void  kickUser                    (QListWidgetItem* pListWidgetItem);
        void  shutdownAllUsers            ();


    // GET functions

        std::string    getServerVersion          ();
        std::string    getLastClientVersion      ();
        unsigned short getPingNormalBelow        ();
        unsigned short getPingWarningBelow       ();

private:

    void catchUDPPackets();
    void eraseUDPPacket();

    void refreshWrongUDPPackets();


    // Closing

        void responseToFIN               (UserStruct* userToClose, bool bUserLost = false);
        void sendFINtoUser               (UserStruct* userToClose);
        void sendFINtoSocket             (SOCKET socketToClose);


    // --------------------------------


    MainWindow*              pMainWindow;
    SettingsManager*         pSettingsManager;
    LogManager*              pLogManager;


    // Users
    SOCKET                   listenTCPSocket;
    SOCKET                   UDPsocket;
    SOCKET                   udpPingCheckSocket;
    std::vector<UserStruct*> users;
    std::vector<UDPPacket*>  vUDPPackets;


    // Ping
    unsigned short           iPingNormalBelow;
    unsigned short           iPingWarningBelow;


    // For wrong or empty packets (not from our users)
    size_t                   iWrongOrEmptyPacketCount; // will refresh every 'clockWrongUDPPacket'
    clock_t                  clockWrongUDPPacket;
    std::mutex               mtxRefreshWrongPacketCount;


    std::mutex               mtxUDPPackets;
    std::mutex               mtxUsersDelete;
    std::mutex               mtxConnectDisconnect;


    std::string              serverVersion;
    std::string              clientLastSupportedVersion;


    int                      iUsersConnectedCount;
    int                      iUsersConnectedToVOIP;


    bool                     bWinSockStarted;
    bool                     bTextListen;
    bool                     bVoiceListen;
};
