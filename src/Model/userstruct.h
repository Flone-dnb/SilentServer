#pragma once

#include <winsock2.h>
#include <string>
#include <vector>
#include <ctime>

class QListWidgetItem;

struct UserStruct
{
    SOCKET userTCPSocket;

    sockaddr_in userUDPAddr;

    clock_t keepAliveTimer;

    std::string userName;
    std::string userIP;
    unsigned short userTCPPort;
    unsigned short iCurrentPing;

    bool bConnectedToTextChat;
    bool bConnectedToVOIP;

    char* pDataFromUser;

    QListWidgetItem* pListItem;
};
