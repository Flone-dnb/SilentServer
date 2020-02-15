// This file is part of the Silent Server.
// Copyright Aleksandr "Flone" Tretyakov (github.com/Flone-dnb).
// Licensed under the ZLib license.
// Refer to the LICENSE file included.

#pragma once


// C++
#include <vector>
#include <string>
#include <ctime>
#include <chrono>
#include <mutex>

// ============== Network ==============
// Sockets and stuff
#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#elif __linux__
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <netdb.h>
#include <fcntl.h>
#include <errno.h>
#endif

#ifdef _WIN32
// Winsock 2 Library
#pragma comment(lib,"Ws2_32.lib")
#endif

#if _WIN32
#define SSocket SOCKET
#elif __linux__
#define SSocket int
#endif

#if _WIN32
#define S16String std::wstring
#define S16Char   wchar_t
#elif __linux__
#define S16String std::u16string
#define S16Char   char16_t
#endif

// Custom
#include "Model/userstruct.h"



class MainWindow;
class SettingsManager;
class LogManager;
class UDPPacket;



// should be shorter than MAX_VERSION_STRING_LENGTH
#define SERVER_VERSION           "2.20.2"
#define CLIENT_SUPPORTED_VERSION "2.23.2"



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

    // UDP related

        void catchUDPPackets                     ();
        void eraseUDPPacket                      ();
        void refreshWrongUDPPackets              ();


    // "Close" function

        void responseToFIN                       (UserStruct* userToClose, bool bUserLost = false);
        void sendFINtoUser                       (UserStruct* userToClose);
        void sendFINtoSocket                     (SSocket socketToClose);
        bool closeSocket                         (SSocket socket);


    // Other

        bool setSocketBlocking                   (SSocket socket, bool bBlocking);
        int  getLastError                        ();


    // --------------------------------


    MainWindow*              pMainWindow;
    SettingsManager*         pSettingsManager;
    LogManager*              pLogManager;


    // Users
    SSocket                  listenTCPSocket;
    SSocket                  UDPsocket;
    SSocket                  udpPingCheckSocket;
    std::vector<UserStruct*> users;
    std::vector<UDPPacket*>  vUDPPackets;


    // Ping
    unsigned short           iPingNormalBelow;
    unsigned short           iPingWarningBelow;
    std::chrono::time_point<std::chrono::steady_clock> pingCheckSendTime;


    // For wrong or empty packets (not from our users)
    size_t                   iWrongOrEmptyPacketCount; // will refresh every 'INTERVAL_REFRESH_WRONG_PACKETS_SEC'
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
