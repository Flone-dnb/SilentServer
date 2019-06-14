#pragma once

// Custom
#include "Model/userstruct.h"

// C++
#include <vector>
#include <mutex>
#include <string>

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

    // Functions related to listening
        void listenForNewConnections();
        void listenForMessage(UserStruct* userToListen);
        void getMessage(UserStruct* userToListen);

    // Functions related to closing something
        void sendFINtoSocket(SOCKET socketToClose);
        void responseToFIN(UserStruct* userToClose, bool bUserLost = false);
        void shutdownAllUsers();

private:

    MainWindow* pMainWindow;

    SOCKET listenSocket;
    std::vector<UserStruct*> users;

    std::mutex mtxUsersWrite;

    int iUsersConnectedCount;

    bool bWinSockStarted;
    bool bListening;


    std::string serverVersion;
};
