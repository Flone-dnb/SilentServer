#pragma once

// Custom
#include "Model/userstruct.h"

// C++
#include <vector>
#include <string>
#include <ctime>

// ============== Network ==============
// Sockets and stuff
#include <winsock2.h>

// Adress translation
#include <ws2tcpip.h>

// Winsock 2 Library
#pragma comment(lib,"Ws2_32.lib")



class MainWindow;


class ServerService
{

public:

    ServerService(MainWindow* pMainWindow);

    std::string getServerVersion();

    // Functions related to starting
        bool startWinSock();
        void startToListenForConnection();
        void prepareForVoiceConnection();

    // Functions related to listening
        void listenForNewConnections();
        void listenForMessage(UserStruct* userToListen);
        void listenForVoiceMessage(UserStruct* userToListen);
        void getMessage(UserStruct* userToListen);

    void checkPing();
    void sendPingToAll(std::string userName, int ping);

    // Functions related to closing something
        void sendFINtoSocket(SOCKET socketToClose);
        void responseToFIN(UserStruct* userToClose, bool bUserLost = false);
        void shutdownAllUsers();

private:

    MainWindow* pMainWindow;

    SOCKET listenSocket;
    SOCKET UDPsocket;
    std::vector<UserStruct*> users;

    int iUsersConnectedCount;
    int iUsersConnectedToVOIP;

    bool bWinSockStarted;
    bool bTextListen;
    bool bVoiceListen;


    std::string serverVersion;
};
