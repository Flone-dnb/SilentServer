#pragma once

// Custom
#include "Model/userstruct.h"

// C++
#include <vector>
#include <mutex>

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

    // Functions related to starting
        void startWinSock();
        void startToListenForConnection();

    // Functions related to listening
        void listenForNewConnections();
        void listenForMessage(UserStruct* userToListen, std::mutex& mtxUsersWrite);

    // Functions related to closing something
        void sendFINtoSocket(SOCKET& socketToClose);
        void responseToFIN(UserStruct* userToClose);
        void shutdownAllUsers();

private:

    MainWindow* pMainWindow;

    SOCKET listenSocket;
    std::vector<UserStruct*> users;

    std::mutex mtxUsersWrite;

    int iUsersConnectedCount;

    bool bWinSockStarted;
    bool bListening;
};
