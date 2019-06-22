#pragma once

#include <winsock2.h>
#include <string>
#include <vector>
#include <ctime>

class QListWidgetItem;

struct UserStruct
{
    SOCKET userTCPSocket;
    sockaddr_in userAddr;

    clock_t keepAliveTimer;

    std::string userName;
    std::string userIP;
    unsigned short userPort;

    bool bConnectedToVOIP;

    char* pDataFromUser;

    QListWidgetItem* pListItem;
};
