// This file is part of the Silent Server.
// Copyright Aleksandr "Flone" Tretyakov (github.com/Flone-dnb).
// Licensed under the ZLib license.
// Refer to the LICENSE file included.

#pragma once


#if _WIN32
#include <winsock2.h>
#elif __linux__
#include <sys/socket.h>
#include <netinet/in.h> // sockaddr_in
#endif


#include <string>
#include <vector>
#include <ctime>
#include <chrono>


#if _WIN32
#define SSocket SOCKET
#elif __linux__
#define SSocket int
#endif


class SListItemUser;



struct UserStruct
{
    SSocket           userTCPSocket;


    sockaddr_in      userUDPAddr;


    std::chrono::time_point<std::chrono::steady_clock> keepAliveTimer;
    std::chrono::time_point<std::chrono::steady_clock> lastTimeMessageSent;


    char*            pDataFromUser;
    SListItemUser*   pUserInList;


    std::string      userName;
    std::string      userIP;


    unsigned short   userTCPPort;
    unsigned short   iPing;


    bool             bConnectedToTextChat;
    bool             bConnectedToVOIP;
};
