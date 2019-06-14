#pragma once

#include <winsock2.h>
#include <string>
#include <vector>
#include <ctime>

class QListWidgetItem;

struct UserStruct
{
    SOCKET userSocket;
    clock_t keepAliveTimer;

    std::string userName;
    std::string userIP;
    unsigned short userPort;

    char* pDataFromUser;

    QListWidgetItem* pListItem;
};
