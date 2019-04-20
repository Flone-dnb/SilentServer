#pragma once

#include <winsock2.h>
#include <string>
#include <vector>

class QListWidgetItem;

struct UserStruct
{
    SOCKET userSocket;

    std::string userName;
    std::string userIP;
    unsigned short userPort;

    char* pDataFromUser;

    QListWidgetItem* pListItem;
};
