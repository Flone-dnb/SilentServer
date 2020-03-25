// This file is part of the Silent Server.
// Copyright Aleksandr "Flone" Tretyakov (github.com/Flone-dnb).
// Licensed under the ZLib license.
// Refer to the LICENSE file included.

#include "serverservice.h"

//STL
#include <thread>

#if _WIN32
using std::memcpy;
#elif __linux__
#include <unistd.h> // for close()
#include <string.h>
#include <time.h>
#include <netinet/tcp.h>
#define SOCKET_ERROR -1
#define INVALID_SOCKET -1
#define SD_SEND SHUT_WR
#endif

//Custom
#include "View/MainWindow/mainwindow.h"
#include "Model/net_params.h"
#include "Model/SettingsManager/settingsmanager.h"
#include "Model/SettingsManager/SettingsFile.h"
#include "Model/LogManager/logmanager.h"
#include "Model/ServerService/UDPPacket.h"

#include "View/CustomList/SListItemUser/slistitemuser.h"
#include "View/CustomList/SListItemRoom/slistitemroom.h"


enum CONNECT_MESSAGES
{
    CM_USERNAME_INUSE  = 0,
    CM_SERVER_FULL     = 2,
    CM_WRONG_CLIENT    = 3,
    CM_SERVER_INFO     = 4,
    CM_NEED_PASSWORD   = 5
};

enum ROOM_COMMAND
{
    RC_ENTER_ROOM           = 15,
    RC_ENTER_ROOM_WITH_PASS = 16,

    RC_CAN_ENTER_ROOM       = 20,
    RC_ROOM_IS_FULL         = 21,
    RC_PASSWORD_REQ         = 22,
    RC_WRONG_PASSWORD       = 23,

    RC_USER_ENTERS_ROOM     = 25,
    RC_SERVER_MOVED_ROOM    = 26,
    RC_SERVER_DELETES_ROOM  = 27,
    RC_SERVER_CREATES_ROOM  = 28,
    RC_SERVER_CHANGES_ROOM  = 29
};

enum TCP_SERVER_MESSAGE
{
    SM_NEW_USER             = 0,
    SM_SOMEONE_DISCONNECTED = 1,
    SM_CAN_START_UDP        = 2,
    SM_SPAM_NOTICE          = 3,
    SM_PING                 = 8,
    SM_KEEPALIVE            = 9,
    SM_USERMESSAGE          = 10,
    SM_KICKED               = 11,
    SM_WRONG_PASSWORD_WAIT  = 12,
    SM_GLOBAL_MESSAGE       = 13
};

enum USER_DISCONNECT_REASON
{
    UDR_DISCONNECT          = 0,
    UDR_LOST                = 1,
    UDR_KICKED              = 2
};

enum UDP_SERVER_MESSAGE
{
    UDP_SM_PREPARE          = -1,
    UDP_SM_PING             = 0,
    UDP_SM_FIRST_PING       = -2,
    UDP_SM_USER_READY       = -3
};



// ------------------------------------------------------------------------------------------------
// ------------------------------------------------------------------------------------------------
// ------------------------------------------------------------------------------------------------



ServerService::ServerService(MainWindow* pMainWindow, SettingsManager* pSettingsManager, LogManager* pLogManager)
{
    this ->pMainWindow         = pMainWindow;
    this ->pSettingsManager    = pSettingsManager;
    this ->pLogManager         = pLogManager;


    // should be shorter than MAX_VERSION_STRING_LENGTH
    serverVersion              = SERVER_VERSION;
    clientLastSupportedVersion = CLIENT_SUPPORTED_VERSION;


    iUsersConnectedCount       = 0;
    iUsersConnectedToVOIP      = 0;
    iWrongOrEmptyPacketCount   = 0;


    bWinSockStarted            = false;
    bTextListen                = false;
}





std::string ServerService::getServerVersion()
{
    return serverVersion;
}

std::string ServerService::getLastClientVersion()
{
    return clientLastSupportedVersion;
}

unsigned short ServerService::getPingNormalBelow()
{
    return iPingNormalBelow;
}

unsigned short ServerService::getPingWarningBelow()
{
    return iPingWarningBelow;
}

void ServerService::sendMessageToAll(const std::string &sMessage)
{
    if (bWinSockStarted && bTextListen && users.size() > 0)
    {
        char vBuffer[MAX_BUFFER_SIZE];
        memset(vBuffer, 0, MAX_BUFFER_SIZE);

        vBuffer[0] = SM_GLOBAL_MESSAGE;

        unsigned short int iMessageSize = static_cast<unsigned short>(sMessage.size());

        std::memcpy(vBuffer + 1, &iMessageSize, sizeof(unsigned short));

        int iIndex = 1 + sizeof(unsigned short);

        std::memcpy(vBuffer + iIndex, sMessage.c_str(), sMessage.size());
        iIndex += sMessage.size();

        int iSentSize = 0;

        mtxUsersDelete.lock();

        for (size_t i = 0; i < users.size(); i++)
        {
            iSentSize = send(users[i]->userTCPSocket, vBuffer, iIndex, 0);

            if (iSentSize != iIndex)
            {
                if (iSentSize == SOCKET_ERROR)
                {
                    pLogManager ->printAndLog( "ServerService::sendMessageToAll::send() function failed and returned: "
                                               + std::to_string(getLastError()), true);
                }
                else
                {
                    pLogManager ->printAndLog( "ServerService::sendMessageToAll::send(): not full sent size on user " + users[i]->userName + ". send() returned: "
                                               + std::to_string(iSentSize), true );
                }
            }
        }

        mtxUsersDelete.unlock();
    }
}

void ServerService::catchUDPPackets()
{
    sockaddr_in senderInfo;
    memset(senderInfo.sin_zero, 0, sizeof(senderInfo.sin_zero));
#if _WIN32
    int iLen = sizeof(senderInfo);
#elif __linux__
    socklen_t iLen = sizeof(senderInfo);
#endif

    char readBuffer[MAX_BUFFER_SIZE];




    // Wrong packet refresh thread

    std::thread tWrongPacketRefresher (&ServerService::refreshWrongUDPPackets, this);
    tWrongPacketRefresher .detach();

    while (bVoiceListen)
    {
        // Peeks at the incoming data. The data is copied into the buffer but is not removed from the input queue.
        int iSize = recvfrom(UDPsocket, readBuffer, MAX_BUFFER_SIZE, MSG_PEEK, reinterpret_cast<sockaddr*>(&senderInfo), &iLen);

        while (iSize >= 0)
        {
            if (iSize == 0)
            {
                // Empty packet, delete it.

                recvfrom(UDPsocket, readBuffer, MAX_BUFFER_SIZE, 0, reinterpret_cast<sockaddr*>(&senderInfo), &iLen);

                mtxRefreshWrongPacketCount .lock();

                iWrongOrEmptyPacketCount++;

                mtxRefreshWrongPacketCount .unlock();
            }
            else
            {
                UDPPacket* pPacket = new UDPPacket();

                pPacket ->iSize = recvfrom(UDPsocket, pPacket ->vPacketData, MAX_BUFFER_SIZE, 0,
                                     reinterpret_cast<sockaddr*>(&pPacket ->senderInfo), &pPacket ->iLen);

                mtxUDPPackets .lock();

                vUDPPackets .push_back(pPacket);

                mtxUDPPackets .unlock();
            }

            iSize = recvfrom(UDPsocket, readBuffer, MAX_BUFFER_SIZE, MSG_PEEK, reinterpret_cast<sockaddr*>(&senderInfo), &iLen);
        }

        std::this_thread::sleep_for( std::chrono::milliseconds(INTERVAL_UDP_MESSAGE_MS / 2) );
    }
}

void ServerService::eraseUDPPacket()
{
    // This function should be called when mtxUDPPackets is locked.

    if ( vUDPPackets[0] ->vThreadsRejectedPacket .size() >= users .size() )
    {
        delete vUDPPackets[0];

        vUDPPackets .erase( vUDPPackets .begin() );


        mtxRefreshWrongPacketCount .lock();

        iWrongOrEmptyPacketCount++;

        mtxRefreshWrongPacketCount .unlock();
    }
}

void ServerService::refreshWrongUDPPackets()
{
    while (bVoiceListen)
    {
        for (size_t i = 0; i < INTERVAL_REFRESH_WRONG_PACKETS_SEC; i++)
        {
            if (bVoiceListen)
            {
               std::this_thread::sleep_for( std::chrono::seconds(1) );
            }
            else
            {
                return;
            }
        }



        if ( iWrongOrEmptyPacketCount > NOTIFY_WHEN_WRONG_PACKETS_MORE )
        {
            pLogManager ->printAndLog("For the last " + std::to_string(INTERVAL_REFRESH_WRONG_PACKETS_SEC) +
                                      " sec. wrong or empty UDP packets received: " +
                                      std::to_string(iWrongOrEmptyPacketCount) + ".\n", true);
        }


        mtxRefreshWrongPacketCount .lock();

        iWrongOrEmptyPacketCount = 0;

        mtxRefreshWrongPacketCount .unlock();
    }
}

bool ServerService::setSocketBlocking(SSocket socket, bool bBlocking)
{
#if _WIN32
    u_long arg = true;

    if (bBlocking)
    {
        arg = false;
    }

    if (ioctlsocket(socket, static_cast<long>(FIONBIO), &arg) == SOCKET_ERROR)
    {
        return true;
    }

    return false;
#elif __linux__
    int flags = fcntl(socket, F_GETFL, 0);
    if (flags == -1)
    {
        return true;
    }

    if (bBlocking)
    {
        flags = flags & ~O_NONBLOCK;
    }
    else
    {
        flags = flags | O_NONBLOCK;
    }

    if (fcntl(socket, F_SETFL, flags) != 0)
    {
        return true;
    }

    return false;
#endif
}

int ServerService::getLastError()
{
#if _WIN32
    return WSAGetLastError();
#elif __linux__
    return errno;
#endif
}

bool ServerService::closeSocket(SSocket socket)
{
#if _WIN32
    if (closesocket(socket))
    {
        return true;
    }
    else
    {
        return false;
    }
#elif __linux__
    if (close(socket))
    {
        return true;
    }
    else
    {
        return false;
    }
#endif
}

bool ServerService::startWinSock()
{
    pMainWindow->clearChatWindow();
    pLogManager ->printAndLog( std::string("Starting...") );

    int result = 0;
#ifdef _WIN32
    // Start Winsock2

    WSADATA WSAData;
    result = WSAStartup(MAKEWORD(2, 2), &WSAData);

    // Start WinSock2 (ver. 2.2)
    if (result != 0)
    {
        pLogManager ->printAndLog(std::string("WSAStartup function failed and returned: " + std::to_string(getLastError()) + ".\nTry again.\n"));
    }
#endif

    if (result == 0)
    {
        bWinSockStarted = true;

        startToListenForConnection();
        if (bTextListen)
        {
            pMainWindow->changeStartStopActionText(true);
            pMainWindow->showSendMessageToAllAction(true);

            std::thread listenThread(&ServerService::listenForNewTCPConnections, this);
            listenThread.detach();

            return true;
        }
    }

    return false;
}

void ServerService::startToListenForConnection()
{
    // Create the IPv4 TCP socket
    listenTCPSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (listenTCPSocket == INVALID_SOCKET)
    {
        pLogManager ->printAndLog("ServerService::listenForConnection()::socket() function failed and returned: " + std::to_string(getLastError()) + ".");
    }
    else
    {
        // Create and fill the "sockaddr_in" structure containing the IPv4 socket
        sockaddr_in myAddr;
        memset(myAddr.sin_zero, 0, sizeof(myAddr.sin_zero));
        myAddr .sin_family      = AF_INET;
        myAddr .sin_port        = htons( pSettingsManager ->getCurrentSettings() ->iPort );
        myAddr .sin_addr.s_addr = INADDR_ANY;

        if (bind(listenTCPSocket, reinterpret_cast<sockaddr*>(&myAddr), sizeof(myAddr)) == SOCKET_ERROR)
        {
            pLogManager ->printAndLog("ServerService::listenForConnection()::bind() function failed and returned: " + std::to_string(getLastError()) + ".\nSocket will be closed. Try again.\n");
            closeSocket(listenTCPSocket);
        }
        else
        {
            // Find out local port and show it
            sockaddr_in myBindedAddr;
#if _WIN32
            int len = sizeof(myBindedAddr);
#elif __linux__
            socklen_t len = sizeof(myBindedAddr);
#endif
            getsockname(listenTCPSocket, reinterpret_cast<sockaddr*>(&myBindedAddr), &len);

            // Get my IP
            char myIP[16];
            inet_ntop(AF_INET, &myBindedAddr.sin_addr, myIP, sizeof(myIP));

            pLogManager ->printAndLog("Success. Waiting for a connection requests on port: " + std::to_string(ntohs(myBindedAddr.sin_port)) + ".\n");

            if (listen(listenTCPSocket, SOMAXCONN) == SOCKET_ERROR)
            {
                pLogManager ->printAndLog(std::string("ServerService::listenForConnection()::listen() function failed and returned: "
                                                      + std::to_string(getLastError()) + ".\nSocket will be closed. Try again.\n"));
                closeSocket(listenTCPSocket);
            }
            else
            {
                pLogManager ->printAndLog("WARNING:\nThe data transmitted over the network is not encrypted.\n");

                // Translate listen socket to non-blocking mode
                if (setSocketBlocking(listenTCPSocket, false))
                {
                    pLogManager ->printAndLog("ServerService::listenForConnection()::setSocketBlocking() failed and returned: "
                                              + std::to_string(getLastError()) + ".\nSocket will be closed. Try again.\n");
                    closeSocket(listenTCPSocket);
                }
                else
                {
                    bTextListen = true;

                    // Prepare for voice connection
                    prepareForVoiceConnection();
                }
            }
        }
    }
}

void ServerService::listenForNewTCPConnections()
{
    sockaddr_in connectedWith;
    memset(connectedWith.sin_zero, 0, sizeof(connectedWith.sin_zero));
#if _WIN32
    int iLen = sizeof(connectedWith);
#elif __linux__
    socklen_t iLen = sizeof(connectedWith);
#endif

    bool bConnectionFinished = true;

    // Accept new connection
    while (bTextListen)
    {
        if (bConnectionFinished == false)
        {
            pLogManager ->printAndLog("The user was not connected.\n", true);

            bConnectionFinished = true;
        }

        // Wait for users to disconnect.
        mtxConnectDisconnect .lock();
        mtxConnectDisconnect .unlock();


        if (bTextListen == false) break;


        SSocket newConnectedSocket;
        newConnectedSocket = accept(listenTCPSocket, reinterpret_cast<sockaddr*>(&connectedWith), &iLen);


        if (newConnectedSocket == INVALID_SOCKET)
        {
            if (bTextListen)
            {
                std::this_thread::sleep_for(std::chrono::milliseconds(INTERVAL_TCP_ACCEPT_MS));

                continue;
            }
            else
            {
                return;
            }
        }




        pLogManager ->printAndLog("\nSomeone is connecting...", true);




        // Disable Nagle algorithm for Connected Socket

#if _WIN32
        BOOL bOptVal = true;
        int bOptLen = sizeof(BOOL);
#elif __linux__
        int bOptVal = 1;
        int bOptLen = sizeof(bOptVal);
#endif
        if (setsockopt(newConnectedSocket, IPPROTO_TCP, TCP_NODELAY, reinterpret_cast<char*>(&bOptVal), bOptLen) == SOCKET_ERROR)
        {
            pLogManager ->printAndLog("ServerService::listenForNewConnections()::setsockopt() (Nagle algorithm) failed and returned: "
                                      + std::to_string(getLastError()) + ".\nSending FIN to this new user.\n",true);

            std::thread closethread(&ServerService::sendFINtoSocket, this, newConnectedSocket);
            closethread.detach();


            bConnectionFinished = false;

            continue;
        }




        // Receive version, user name and password (optional)

        const size_t iBufferLength = MAX_NAME_LENGTH + MAX_VERSION_STRING_LENGTH + (UCHAR_MAX * 2) + 2;
        char nameBuffer[iBufferLength];
        memset(nameBuffer, 0, iBufferLength);

        recv(newConnectedSocket, nameBuffer, iBufferLength, 0);




        // Received version, user name and password

        // Check if client version is the same with the server version
        char clientVersionSize = nameBuffer[0];

        char* pVersion = new char[ static_cast<size_t>(clientVersionSize + 1) ];
        memset( pVersion, 0, static_cast<size_t>(clientVersionSize + 1) );

        memcpy(pVersion, nameBuffer + 1, static_cast<size_t>(clientVersionSize));

        std::string clientVersion(pVersion);
        delete[] pVersion;



        if ( clientVersion != clientLastSupportedVersion )
        {
            pLogManager ->printAndLog("Client version \"" + clientVersion + "\" does not match with the last supported client version "
                                      + clientLastSupportedVersion + ".\n", true);
            char answerBuffer[MAX_VERSION_STRING_LENGTH + 2];
            memset(answerBuffer, 0, MAX_VERSION_STRING_LENGTH + 2);

            answerBuffer[0] = CM_WRONG_CLIENT;
            answerBuffer[1] = static_cast<char>(clientLastSupportedVersion.size());
            memcpy(answerBuffer + 2, clientLastSupportedVersion.c_str(), clientLastSupportedVersion.size());

            send(newConnectedSocket, answerBuffer, static_cast<int>(2 + clientLastSupportedVersion.size()), 0);
            std::thread closethread(&ServerService::sendFINtoSocket, this, newConnectedSocket);
            closethread.detach();

            bConnectionFinished = false;

            continue;
        }





        // Check if this user name is free

        char vBuffer[MAX_NAME_LENGTH + 2];
        memset(vBuffer, 0, MAX_NAME_LENGTH + 2);

        memcpy(vBuffer, nameBuffer + 2 + clientVersionSize, static_cast <size_t> (nameBuffer[1 + clientVersionSize]));

        //std::string userNameStr(nameBuffer + 1 + clientVersionSize);
        std::string userNameStr(vBuffer);
        bool bUserNameFree = true;


        mtxConnectDisconnect .lock();


        for (unsigned int i = 0;   i< users .size();   i++)
        {
            if (users[i]->userName == userNameStr)
            {
                bUserNameFree = false;
                break;
            }
        }


        mtxConnectDisconnect .unlock();


        if (bUserNameFree == false)
        {
            pLogManager ->printAndLog("User name " + userNameStr + " is already taken.",true);
            char command = CM_USERNAME_INUSE;
            send(newConnectedSocket,reinterpret_cast<char*>(&command), 1, 0);
            std::thread closethread(&ServerService::sendFINtoSocket, this, newConnectedSocket);
            closethread.detach();


            bConnectionFinished = false;

            continue;
        }



        // Check if the password is right.

        if (pSettingsManager ->getCurrentSettings() ->sPasswordToJoin != u"")
        {
            char16_t vPassBuffer[UCHAR_MAX + 1];
            memset(vPassBuffer, 0, (UCHAR_MAX * 2) + 2);

            char cUserNameSize = nameBuffer[1 + clientVersionSize];
            unsigned char cPasswordSize = static_cast <unsigned char> (nameBuffer[2 + clientVersionSize + cUserNameSize]);

            memcpy(vPassBuffer, nameBuffer + 3 + clientVersionSize + cUserNameSize,
                        static_cast<size_t>(cPasswordSize) * 2);

            std::u16string sPassword(vPassBuffer);

            if ( pSettingsManager ->getCurrentSettings() ->sPasswordToJoin != sPassword )
            {
                pLogManager ->printAndLog("User " + userNameStr + " entered wrong or blank password.",true);
                char command = CM_NEED_PASSWORD;
                send(newConnectedSocket,reinterpret_cast<char*>(&command), 1, 0);
                std::thread closethread(&ServerService::sendFINtoSocket, this, newConnectedSocket);
                closethread.detach();


                bConnectionFinished = false;

                continue;
            }
        }



        // Show with whom connected

        char connectedWithIP[16];
        memset(&connectedWithIP,0,16);
        inet_ntop(AF_INET, &connectedWith.sin_addr, connectedWithIP, sizeof(connectedWithIP));


        const int iMaxBufferSize = MAX_TCP_BUFFER_SIZE;

        char tempData[iMaxBufferSize];
        memset(tempData, 0, iMaxBufferSize);

        // we ++ new user (if something will go wrong later we will -- this user
        iUsersConnectedCount++;


        // Prepare online info to user.
        // Prepared data format (amount of bytes in '()'):

        // (1) Is user name free (if not then all other stuff is not included)
        // (2) Packet size minus "free name" byte
        // (4) Amount of users in main lobby (online)
        // [
        //      (1) Size in bytes of user name online
        //      (usernamesize) user name
        // ]

        int iBytesWillSend = 0;
        tempData[0] = CM_SERVER_INFO;
        iBytesWillSend++;

        // We will put here packet size
        iBytesWillSend += 2;


        mtxRooms.lock();


        std::vector<std::string> vRooms = pMainWindow->getRoomNames();
		
        char roomsCount = static_cast<char>(vRooms.size());
		memcpy(tempData + iBytesWillSend, &roomsCount, 1);
		iBytesWillSend++;

        for (size_t i = 0; i < vRooms.size(); i++)
        {
            char cRoomNameSize = static_cast<char>(vRooms[i].size());
            memcpy(tempData + iBytesWillSend, &cRoomNameSize, 1);
            iBytesWillSend++;

            memcpy(tempData + iBytesWillSend, vRooms[i].c_str(), static_cast<size_t>(cRoomNameSize));
            iBytesWillSend += cRoomNameSize;



            unsigned short iMaxUsers = pMainWindow->getRoomMaxUsers(i);

            memcpy(tempData + iBytesWillSend, &iMaxUsers, 2);
            iBytesWillSend += 2;



            // Copy users in room.

            std::vector<std::string> vUsers = pMainWindow->getUsersOfRoomIndex(i);

            unsigned short iUsersInRoom = static_cast<unsigned short>(vUsers.size());
            memcpy(tempData + iBytesWillSend, &iUsersInRoom, sizeof(iUsersInRoom));
            iBytesWillSend += sizeof(iUsersInRoom);

            for (size_t j = 0; j < vUsers.size(); j++)
            {
                char nameSize = static_cast<char>(vUsers[j].size());
                memcpy(tempData + iBytesWillSend, &nameSize, 1);
                iBytesWillSend++;

                memcpy(tempData + iBytesWillSend, vUsers[j].c_str(), static_cast<size_t>(nameSize));
                iBytesWillSend += nameSize;
            }
        }

        mtxRooms.unlock();

        // Put packet size to buffer (packet size - command size (1 byte) - packet size (2 bytes))
        unsigned short int iPacketSize = static_cast<unsigned short>(iBytesWillSend - 3);
        memcpy(tempData + 1, &iPacketSize, 2);


        if (iBytesWillSend > iMaxBufferSize)
        {
            // This should happen when you got like >200 users online (and all users have name long 20 chars) if my calculations are correct.

            pLogManager ->printAndLog("Server is full.\n", true);

            char serverIsFullCommand = CM_SERVER_FULL;
            send(newConnectedSocket,reinterpret_cast<char*>(&serverIsFullCommand), 1, 0);
            std::thread closethread(&ServerService::sendFINtoSocket, this, newConnectedSocket);
            closethread.detach();


            bConnectionFinished = false;

            continue;
        }


        // SEND
        int iBytesWereSent = send(newConnectedSocket, tempData, iBytesWillSend, 0);

        if (iBytesWereSent != iBytesWillSend)
        {
            pLogManager ->printAndLog("WARNING:\n" + std::to_string(iBytesWereSent) + " bytes were sent of total "
                                      + std::to_string(iBytesWillSend) + " to new user.\n", true);
        }
        if (iBytesWereSent == -1)
        {
            pLogManager ->printAndLog("ServerService::listenForNewConnections()::send()) (online info) failed and returned: "
                                      + std::to_string(getLastError()) + ".", true);

            if (recv(newConnectedSocket, tempData, MAX_BUFFER_SIZE, 0) == 0)
            {
                pLogManager ->printAndLog("Received FIN from this new user who didn't receive online info.", true);

                shutdown(newConnectedSocket, SD_SEND);
                if (closeSocket(newConnectedSocket) == false)
                {
                    pLogManager ->printAndLog("Closed this socket with success.", true);
                }
                else
                {
                    pLogManager ->printAndLog("Can't close this socket... You better reboot the server.", true);
                }
            }

            iUsersConnectedCount--;

            bConnectionFinished = false;

            continue;
        }



        // Translate new connected socket to non-blocking mode
        if (setSocketBlocking(newConnectedSocket, false))
        {
            pLogManager ->printAndLog("ServerService::listenForNewConnections()::setSocketBlocking() (non-blocking mode) failed and returned: " + std::to_string(getLastError()) + ".", true);

            std::thread closethread(&ServerService::sendFINtoSocket, this, newConnectedSocket);
            closethread.detach();


            bConnectionFinished = false;

            continue;
        }


        mtxConnectDisconnect .lock();


        if (users.size() != 0)
        {
            // Tell other users about new user

            char newUserInfo[MAX_NAME_LENGTH + 11];
            memset(newUserInfo, 0, MAX_NAME_LENGTH + 11);

            unsigned char sizeOfUserName = static_cast<unsigned char>(userNameStr.size());

            unsigned char iSendSize = 0;

            unsigned char commandType = SM_NEW_USER;
            memcpy(newUserInfo, &commandType, 1);
            iSendSize++;

            // Put packet size
            unsigned char iPacketSize1 = 4 + 1 + sizeOfUserName;
            memcpy(newUserInfo + 1, &iPacketSize1, 1);
            iSendSize++;

            memcpy(newUserInfo + 2, &iUsersConnectedCount, 4);
            iSendSize += 4;

            memcpy(newUserInfo + 6, &sizeOfUserName, 1);
            iSendSize++;
            memcpy(newUserInfo + 7, userNameStr.c_str(), sizeOfUserName);
            iSendSize += sizeOfUserName;

            // Send this data
            for (unsigned int i = 0; i < users.size(); i++)
            {
                if ( send(users[i]->userTCPSocket, newUserInfo, iSendSize, 0) != iSendSize)
                {
                    pLogManager ->printAndLog("ServerService::listenForNewTCPConnections::send() failed (info about new user).", true );
                }
            }
        }


        // Fill UserStruct for new user

        UserStruct* pNewUser          = new UserStruct();
        pNewUser->userTCPSocket       = newConnectedSocket;
        pNewUser->userName            = userNameStr;
        pNewUser->pDataFromUser       = new char[MAX_BUFFER_SIZE];
        pNewUser->userIP              = std::string(connectedWithIP);
        pNewUser->userTCPPort         = ntohs(connectedWith.sin_port);
        pNewUser->keepAliveTimer      = std::chrono::steady_clock::now();
        pNewUser->lastTimeMessageSent = std::chrono::steady_clock::now();
        pNewUser->lastTimeWrongPasswordEntered = std::chrono::steady_clock::now();

        memset(pNewUser->userUDPAddr.sin_zero, 0, sizeof(pNewUser->userUDPAddr.sin_zero));
        pNewUser->userUDPAddr.sin_family = AF_INET;
        pNewUser->userUDPAddr.sin_addr   = connectedWith.sin_addr;
        pNewUser->userUDPAddr.sin_port   = 0;
        pNewUser->iPing                  = 0;

        pNewUser->bConnectedToTextChat   = false;
        pNewUser->bConnectedToVOIP       = false;

        users.push_back(pNewUser);

        // Ready to send and receive data

        pLogManager ->printAndLog("Connected with " + std::string(connectedWithIP) + ":" + std::to_string(ntohs(connectedWith.sin_port)) + " AKA " + pNewUser->userName + ".",true);
        pMainWindow->updateOnlineUsersCount(iUsersConnectedCount);
        pNewUser->pUserInList = pMainWindow->addNewUserToList(pNewUser->userName);

        std::thread listenThread(&ServerService::listenForMessage, this, pNewUser);
        listenThread.detach();


        mtxConnectDisconnect .unlock();
    }
}

void ServerService::prepareForVoiceConnection()
{
    UDPsocket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (UDPsocket == INVALID_SOCKET)
    {
        pLogManager ->printAndLog( "ServerService::prepareForVoiceConnection::socket() error: " + std::to_string(getLastError()), true );
        return;
    }

    sockaddr_in myAddr;
    memset(myAddr.sin_zero, 0, sizeof(myAddr.sin_zero));
    myAddr.sin_family = AF_INET;
    myAddr.sin_port = htons(pSettingsManager ->getCurrentSettings() ->iPort);
    myAddr.sin_addr.s_addr = INADDR_ANY;



    // Allows the socket to be bound to an address that is already in use.
#if _WIN32
    BOOL bMultAddr = true;
#elif __linux__
    int bMultAddr = 1;
#endif

    if (setsockopt(UDPsocket, SOL_SOCKET, SO_REUSEADDR, reinterpret_cast<char*>(&bMultAddr), sizeof(bMultAddr)) == SOCKET_ERROR)
    {
        pLogManager ->printAndLog( "ServerService::prepareForVoiceConnection::setsockopt() error: " + std::to_string(getLastError()), true );
        closeSocket(UDPsocket);
        return;
    }




    if (bind(UDPsocket, reinterpret_cast<sockaddr*>(&myAddr), sizeof(myAddr)) == SOCKET_ERROR)
    {
        pLogManager ->printAndLog( "ServerService::prepareForVoiceConnection::bind() error: " + std::to_string(getLastError()), true );
        closeSocket(UDPsocket);
        return;
    }



    // Translate listen socket to non-blocking mode
    if (setSocketBlocking(UDPsocket, false))
    {
        pLogManager ->printAndLog( "ServerService::prepareForVoiceConnection::setSocketBlocking() error: " + std::to_string(getLastError()), true );
        closeSocket(UDPsocket);
        return;
    }



    bVoiceListen = true;



    std::thread tUDP (&ServerService::catchUDPPackets, this);
    tUDP .detach();
}

void ServerService::listenForMessage(UserStruct* userToListen)
{
    userToListen->bConnectedToTextChat = true;

    std::thread udpThread(&ServerService::listenForVoiceMessage, this, userToListen);
    udpThread.detach();

    while(userToListen->bConnectedToTextChat)
    {
        while (recv(userToListen->userTCPSocket, userToListen->pDataFromUser, 0, 0) == 0)
        {
            // There are some data to read

            int receivedAmount = recv(userToListen->userTCPSocket, userToListen->pDataFromUser, 1, 0);
            if (receivedAmount == 0)
            {
                // Client sent FIN

                responseToFIN(userToListen);

                // Stop thread
                return;
            }
            else
            {
                userToListen->keepAliveTimer = std::chrono::steady_clock::now();

                if (userToListen->pDataFromUser[0] == SM_USERMESSAGE)
                {
                    // This is a message (in main lobby), send it to all in main lobby

                    processMessage(userToListen);
                }
                else if ( (userToListen->pDataFromUser[0] == RC_ENTER_ROOM)
                          || (userToListen->pDataFromUser[0] == RC_ENTER_ROOM_WITH_PASS) )
                {
                    checkRoomSettings(userToListen);
                }
            }
        }

        float timePassedInSeconds = std::chrono::duration_cast<std::chrono::seconds>(std::chrono::steady_clock::now() - userToListen->keepAliveTimer).count();

        if (timePassedInSeconds > INTERVAL_KEEPALIVE_SEC)
        {
            // User was inactive for INTERVAL_KEEPALIVE_SEC seconds
            // Check if he's alive

            char keepAliveChar = SM_KEEPALIVE;
            send(userToListen->userTCPSocket, &keepAliveChar, 1, 0);

            // Translate user socket to blocking mode
            setSocketBlocking(userToListen->userTCPSocket, true);

            // Set recv() time out time to 10 seconds
#if _WIN32
            DWORD time = MAX_TIMEOUT_TIME_MS;

            if (setsockopt(userToListen->userTCPSocket, SOL_SOCKET, SO_RCVTIMEO, reinterpret_cast<char*>(&time), sizeof(time))
                    < 0)
            {
                pLogManager->printAndLog("ServerService::listenForMessage::setsockopt() failed.", true);
            }
#elif __linux__
            struct timeval timeout;
            timeout.tv_sec = MAX_TIMEOUT_TIME_MS / 1000;
            timeout.tv_usec = 0;

            if (setsockopt(userToListen->userTCPSocket, SOL_SOCKET, SO_RCVTIMEO, reinterpret_cast<char*>(&timeout), sizeof(timeout))
                           < 0)
            {
                pLogManager->printAndLog("ServerService::listenForMessage::setsockopt() failed.", true);
            }
#endif


            keepAliveChar = 0;
            int returnCode = recv(userToListen->userTCPSocket, &keepAliveChar, 1, 0);
            if ((keepAliveChar == 9) && returnCode >= 0)
            {
                userToListen->keepAliveTimer = std::chrono::steady_clock::now();

                // Translate user socket to non-blocking mode
                setSocketBlocking(userToListen->userTCPSocket, false);

                if (keepAliveChar == 10) processMessage(userToListen);
                else if (keepAliveChar == 0) responseToFIN(userToListen);
            }
            else
            {
                // We lost connection with this user
                responseToFIN(userToListen, true);

                // Stop thread
                return;
            }
        }

        for (size_t i = 0;   i < INTERVAL_TCP_ACCEPT_MS / 25;   i++)
        {
            if (userToListen ->bConnectedToTextChat)
            {
               std::this_thread::sleep_for(std::chrono::milliseconds(25));
            }
            else
            {
                return;
            }
        }
        //std::this_thread::sleep_for(std::chrono::milliseconds(INTERVAL_TCP_MESSAGE_MS));
    }
}

void ServerService::listenForVoiceMessage(UserStruct *userToListen)
{
    // Preparation cycle
    while( (userToListen ->bConnectedToTextChat) && (bTextListen) )
    {
        mtxUDPPackets .lock();

        if (vUDPPackets .size() == 0)
        {
            mtxUDPPackets .unlock();

            std::this_thread::sleep_for( std::chrono::milliseconds(INTERVAL_UDP_MESSAGE_MS) );

            continue;
        }
        else
        {
            mtxUsersDelete .lock();

            if (userToListen ->bConnectedToTextChat == false)
            {
                mtxUDPPackets .unlock();

                return;
            }

            mtxUsersDelete .unlock();
        }


        UDPPacket* pPacket = vUDPPackets[0];

        // If it's data not from 'userToListen' user then we should not touch it.

#if _WIN32
        if ( (pPacket ->vPacketData[0] == UDP_SM_PREPARE)
             && (pPacket ->senderInfo.sin_addr.S_un.S_un_b.s_b1 == userToListen->userUDPAddr.sin_addr.S_un.S_un_b.s_b1)
             && (pPacket ->senderInfo.sin_addr.S_un.S_un_b.s_b2 == userToListen->userUDPAddr.sin_addr.S_un.S_un_b.s_b2)
             && (pPacket ->senderInfo.sin_addr.S_un.S_un_b.s_b3 == userToListen->userUDPAddr.sin_addr.S_un.S_un_b.s_b3)
             && (pPacket ->senderInfo.sin_addr.S_un.S_un_b.s_b4 == userToListen->userUDPAddr.sin_addr.S_un.S_un_b.s_b4) )
        {
#elif __linux__
        if ( (pPacket ->vPacketData[0] == UDP_SM_PREPARE)
             && (pPacket ->senderInfo.sin_addr.s_addr == userToListen->userUDPAddr.sin_addr.s_addr) )
        {
#endif
            char userNameSize = pPacket ->vPacketData[1];
            char userNameBuffer[MAX_NAME_LENGTH + 1];
            memset(userNameBuffer, 0, MAX_NAME_LENGTH + 1);
            memcpy(userNameBuffer, pPacket ->vPacketData + 2, static_cast<size_t>(userNameSize));

            if ( std::string(userNameBuffer) == userToListen->userName )
            {
                vUDPPackets .erase( vUDPPackets .begin() );

                mtxUDPPackets .unlock();


                userToListen->userUDPAddr.sin_port = pPacket ->senderInfo.sin_port;

                // Tell user
                char readyForVOIPcode = SM_CAN_START_UDP;
                send(userToListen->userTCPSocket, &readyForVOIPcode, 1, 0);

                // Ready to check ping
                userToListen->bConnectedToVOIP = true;

                iUsersConnectedToVOIP++;
                pLogManager ->printAndLog( "We are ready for VOIP with " + userToListen->userName + ".\n", true );

                if (iUsersConnectedToVOIP == 1)
                {
                    std::thread pingCheckThread(&ServerService::checkPing, this);
                    pingCheckThread.detach();
                }


                delete pPacket;


                break;
            }
            else
            {
                mtxUsersDelete .lock();

                if ( (userToListen ->bConnectedToTextChat)
                     &&
                     (pPacket ->checkRejected(userToListen ->userName) == false) )
                {
                    pPacket ->rejectPacket( userToListen ->userName );

                    eraseUDPPacket();
                }

                mtxUsersDelete .unlock();

                mtxUDPPackets .unlock();

                if (userToListen ->bConnectedToTextChat)
                {
                    std::this_thread::sleep_for( std::chrono::milliseconds(INTERVAL_UDP_MESSAGE_MS) );

                    continue;
                }
                else
                {
                    return;
                }
            }
        }
        else
        {
            mtxUsersDelete .lock();

            if ( (userToListen ->bConnectedToTextChat)
                 &&
                 (pPacket ->checkRejected(userToListen ->userName) == false) )
            {
                pPacket ->rejectPacket( userToListen ->userName );

                eraseUDPPacket();
            }

            mtxUsersDelete .unlock();

            mtxUDPPackets .unlock();

            if (userToListen ->bConnectedToTextChat)
            {
                std::this_thread::sleep_for( std::chrono::milliseconds(INTERVAL_UDP_MESSAGE_MS) );

                continue;
            }
            else
            {
                return;
            }
        }
    }




    std::chrono::time_point<std::chrono::steady_clock> firstPingCheckSendTime;



    while ( (userToListen ->bConnectedToVOIP) && (bTextListen) )
    {
        mtxUDPPackets .lock();

        if ( (vUDPPackets .size() == 0) || (userToListen->bConnectedToVOIP == false))
        {
            mtxUDPPackets .unlock();

            if (userToListen ->bConnectedToVOIP == false) return;

            std::this_thread::sleep_for( std::chrono::milliseconds(INTERVAL_UDP_MESSAGE_MS) );

            continue;
        }
        else
        {
            mtxUsersDelete .lock();

            if (userToListen ->bConnectedToVOIP == false)
            {
                mtxUDPPackets .unlock();

                return;
            }

            mtxUsersDelete .unlock();
        }


        UDPPacket* pPacket = vUDPPackets[0];

        // If it's data not from 'userToListen' user then we should not touch it.

#if _WIN32
        while ( (pPacket ->senderInfo.sin_addr.S_un.S_un_b.s_b1 == userToListen->userUDPAddr.sin_addr.S_un.S_un_b.s_b1)
             && (pPacket ->senderInfo.sin_addr.S_un.S_un_b.s_b2 == userToListen->userUDPAddr.sin_addr.S_un.S_un_b.s_b2)
             && (pPacket ->senderInfo.sin_addr.S_un.S_un_b.s_b3 == userToListen->userUDPAddr.sin_addr.S_un.S_un_b.s_b3)
             && (pPacket ->senderInfo.sin_addr.S_un.S_un_b.s_b4 == userToListen->userUDPAddr.sin_addr.S_un.S_un_b.s_b4)
             && (userToListen ->userUDPAddr.sin_port == pPacket ->senderInfo.sin_port) )
        {
#elif __linux__
        while ( (pPacket ->senderInfo.sin_addr.s_addr == userToListen->userUDPAddr.sin_addr.s_addr)
             && (userToListen ->userUDPAddr.sin_port == pPacket ->senderInfo.sin_port) )
        {
#endif
            vUDPPackets .erase( vUDPPackets .begin() );

            mtxUDPPackets .unlock();


            if (pPacket ->vPacketData[0] == UDP_SM_PING)
            {
                // it's ping check
                userToListen ->iPing = static_cast<unsigned short>(std::chrono::duration_cast<std::chrono::milliseconds>
                        (std::chrono::steady_clock::now() - pingCheckSendTime).count());
            }
            else if (pPacket ->vPacketData[0] == UDP_SM_USER_READY)
            {
                // Send first ping check.

                char pingPacket = UDP_SM_FIRST_PING;
                firstPingCheckSendTime = std::chrono::steady_clock::now();

                int iSendPacketSize = sendto(UDPsocket, &pingPacket, sizeof(pingPacket), 0,
                                             reinterpret_cast<sockaddr*>(&userToListen->userUDPAddr), sizeof(userToListen->userUDPAddr));

                if (iSendPacketSize != sizeof(pingPacket))
                {
                    pLogManager ->printAndLog( userToListen->userName + "'s first ping check was not sent! "
                                               "ServerService::listenForVoiceMessage()::sendto() failed and returned: "
                                               + std::to_string(getLastError()) + ".\n", true);
                }
            }
            else if (pPacket ->vPacketData[0] == UDP_SM_FIRST_PING)
            {
                userToListen ->iPing = static_cast<unsigned short>(std::chrono::duration_cast<std::chrono::milliseconds>
                        (std::chrono::steady_clock::now() - firstPingCheckSendTime).count());

                sendPingToAll(userToListen);
            }
            else
            {
                char vBuffer[MAX_BUFFER_SIZE + MAX_NAME_LENGTH + 2];
                memset(vBuffer, 0, MAX_BUFFER_SIZE + MAX_NAME_LENGTH + 2);

                memcpy( vBuffer + 1 + userToListen ->userName .size(), pPacket ->vPacketData,
                             static_cast <size_t> (pPacket ->iSize) );

                vBuffer[0] = static_cast<char>(userToListen->userName.size());
                memcpy(vBuffer + 1, userToListen->userName.c_str(), userToListen->userName.size());

                int iSize = pPacket ->iSize;
                iSize    += 1 + static_cast <int> (userToListen->userName.size());

                int iSentSize = 0;

                for (unsigned int i = 0; i < users.size(); i++)
                {
                    if ( (userToListen->pUserInList->getRoom() == users[i]->pUserInList->getRoom())
                         && (users[i]->userName != userToListen->userName) )
                    {
                        iSentSize = sendto(UDPsocket, vBuffer, iSize, 0, reinterpret_cast<sockaddr*>(&users[i]->userUDPAddr), sizeof(users[i]->userUDPAddr));
                        if (iSentSize != iSize)
                        {
                            if (iSentSize == SOCKET_ERROR)
                            {
                                pLogManager ->printAndLog( userToListen->userName + "'s voice message has not been sent to "
                                                           + users[i]->userName + "!\n"
                                                           "ServerService::listenForVoiceMessage()::sendto() failed and returned: "
                                                           + std::to_string(getLastError()) + ".\n", true);
                            }
                            else
                            {
                                pLogManager ->printAndLog( userToListen->userName + "'s voice message has not been fully sent to "
                                                           + users[i]->userName + "!\n", true);
                            }
                        }
                    }
                }
            }


            delete pPacket;



            if (userToListen->bConnectedToVOIP == false)
            {
                return;
            }


            mtxUDPPackets .lock();

            if (vUDPPackets .size() == 0)
            {
                pPacket = nullptr;

                mtxUDPPackets .unlock();

                break;
            }
            else
            {
                pPacket = vUDPPackets[0];
            }
        }


        if (pPacket)
        {
            // Not our packet

            mtxUsersDelete .lock();

            if ( (userToListen ->bConnectedToVOIP)
                 &&
                 (pPacket ->checkRejected(userToListen ->userName) == false) )
            {
                pPacket ->rejectPacket( userToListen ->userName );

                eraseUDPPacket();
            }

            mtxUsersDelete .unlock();

            mtxUDPPackets .unlock();
        }


        if (userToListen->bConnectedToVOIP == false)
        {
            return;
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(INTERVAL_UDP_MESSAGE_MS));
    }
}

void ServerService::processMessage(UserStruct *userToListen)
{
    // Get message size.
    unsigned short int iMessageSize = 0;
    recv(userToListen->userTCPSocket, reinterpret_cast<char*>(&iMessageSize), 2, 0);



    // Check if the message is sent too quickly.
    float timePassedInSeconds = std::chrono::duration_cast<std::chrono::milliseconds>
            (std::chrono::steady_clock::now() - userToListen ->lastTimeMessageSent).count() / 1000.0f;

    if ( timePassedInSeconds < ANTI_SPAM_MINIMUM_TIME_SEC )
    {
        // Receive user message.
        char16_t vMessageBuffer[MAX_BUFFER_SIZE / 2];
        memset(vMessageBuffer, 0, MAX_BUFFER_SIZE / 2);

        recv(userToListen->userTCPSocket, reinterpret_cast <char*> (vMessageBuffer), iMessageSize, 0);



        char cSpamNotice = SM_SPAM_NOTICE;

        send(userToListen->userTCPSocket, &cSpamNotice, sizeof(cSpamNotice), 0);

        return;
    }
    else
    {
        userToListen->lastTimeMessageSent = std::chrono::steady_clock::now();
    }


    // Get local time.
    time_t now = time(nullptr);
    struct tm timeinfo;
#if _WIN32
    localtime_s(&timeinfo, &now);
#elif __linux__
    timeinfo = *localtime(&now);
#endif


    // Create string to send in format: "Hour:Minute. UserName: Message".

    std::string timeString = "";

    if (std::to_string(timeinfo.tm_hour).size() == 1)
    {
        timeString += "0";
    }
    timeString += std::to_string(timeinfo.tm_hour);
    timeString += ":";
    if (std::to_string(timeinfo.tm_min).size() == 1)
    {
        timeString += "0";
    }
    timeString += std::to_string(timeinfo.tm_min);
    timeString += ". ";
    timeString += userToListen->userName;
    timeString += ": ";



    // Add 'timeString' size and 'iMessageSize' to 'iPacketSize'
    unsigned short int iPacketSize = static_cast<unsigned short int>(timeString.size() + iMessageSize);

    // Prepare buffer to send
    char* pSendToAllBuffer = new char[3 + iPacketSize + 1];
    memset(pSendToAllBuffer, 0, 3 + iPacketSize + 1);

    // Set packet ID (message) to buffer
    pSendToAllBuffer[0] = SM_USERMESSAGE;
    // Set packet size to buffer
    memcpy(pSendToAllBuffer + 1, &iPacketSize, 2);
    // Copy time and name to buffer
    memcpy(pSendToAllBuffer + 3, timeString.c_str(), timeString.size());




    // Receive user message to send
    char16_t vMessageBuffer[MAX_BUFFER_SIZE / 2];
    memset(vMessageBuffer, 0, MAX_BUFFER_SIZE / 2);


    //recv(userToListen->userTCPSocket, pSendToAllBuffer + 3 + timeString.size(), iMessageSize, 0);
    recv(userToListen->userTCPSocket, reinterpret_cast <char*> (vMessageBuffer), iMessageSize, 0);

    std::u16string sUserMessage (vMessageBuffer);

    if (pSettingsManager ->getCurrentSettings() ->bAllowHTMLInMessages == false)
    {
        for (size_t i = 0;   i < sUserMessage .size();   i++)
        {
            if ( sUserMessage[i] == L'<' || sUserMessage[i] == L'>' )
            {
                sUserMessage .erase( sUserMessage .begin() + static_cast <long long> (i) );
                i--;
            }
        }
    }

    memcpy(pSendToAllBuffer + 3 + timeString .size(), sUserMessage .c_str(), iMessageSize);

    int returnCode = 0;



    // Send message to all but not the sender
    for (unsigned int j = 0; j < users.size(); j++)
    {
        if (userToListen->pUserInList->getRoom() == users[j]->pUserInList->getRoom())
        {
            returnCode = send(users[j]->userTCPSocket, pSendToAllBuffer, 3 + iPacketSize, 0);
            if ( returnCode != (3 + iPacketSize) )
            {
                if (returnCode == SOCKET_ERROR)
                {
                    pLogManager ->printAndLog( "ServerService::getMessage::send() function failed and returned: " + std::to_string(getLastError()), true );
                }
                else
                {
                    pLogManager ->printAndLog( userToListen->userName + "'s message wasn't fully sent. send() returned: " + std::to_string(returnCode), true );
                }
            }
        }
    }

    delete[] pSendToAllBuffer;
}

void ServerService::checkRoomSettings(UserStruct *userToListen)
{
    bool bWithPassword = false;

    if (userToListen->pDataFromUser[0] == RC_ENTER_ROOM_WITH_PASS)
    {
        bWithPassword = true;
    }

    char cRoomNameSize = 0;

    recv(userToListen->userTCPSocket, &cRoomNameSize, 1, 0);

    char vBuffer[MAX_NAME_LENGTH + 1];
    memset(vBuffer, 0, MAX_NAME_LENGTH + 1);

    recv(userToListen->userTCPSocket, vBuffer, cRoomNameSize, 0);

    std::string sRoomNameStr = vBuffer;
    std::u16string sPassword = u"";

    if (bWithPassword)
    {
        char16_t vPasswordBuffer[MAX_NAME_LENGTH + 1];
        memset(vPasswordBuffer, 0, (MAX_NAME_LENGTH + 1) * sizeof(char16_t));

        char cPasswordSize = 0;
        recv(userToListen->userTCPSocket, &cPasswordSize, 1, 0);

        cPasswordSize *= 2;

        recv(userToListen->userTCPSocket, reinterpret_cast<char*>(vPasswordBuffer), cPasswordSize, 0);

        sPassword = vPasswordBuffer;
    }

    bool bRoomFull = false;
    bool bPasswordNeeded = false;



    mtxRooms.lock();

    bool bResult = pMainWindow->checkRoomSettings(sRoomNameStr, &bPasswordNeeded, &bRoomFull);

    if (bResult == false)
    {
        char vSendBuffer[MAX_NAME_LENGTH + 3];
        memset(vSendBuffer, 0, MAX_NAME_LENGTH + 3);

        if (bPasswordNeeded == false && bRoomFull == false)
        {
            userEntersRoom(userToListen, sRoomNameStr);
        }
        else if (bRoomFull)
        {
            memset(vBuffer, 0, MAX_NAME_LENGTH + 1);

            vBuffer[0] = RC_ROOM_IS_FULL;

            send(userToListen->userTCPSocket, &vBuffer[0], 1, 0);
        }
        else
        {
            if (sPassword != u"")
            {
                float timePassedInSeconds = std::chrono::duration_cast<std::chrono::milliseconds>
                        (std::chrono::steady_clock::now() - userToListen ->lastTimeWrongPasswordEntered).count() / 1000.0f;

                if (timePassedInSeconds <= WRONG_PASSWORD_INTERVAL_SEC)
                {
                    char cRes = SM_WRONG_PASSWORD_WAIT;

                    send(userToListen->userTCPSocket, &cRes, 1, 0);
                }
                else
                {
                    std::u16string sRoomPassword = pMainWindow->getRoomPassword(sRoomNameStr);

                    if (sPassword == sRoomPassword)
                    {
                        userEntersRoom(userToListen, sRoomNameStr);
                    }
                    else
                    {
                        vSendBuffer[0] = RC_WRONG_PASSWORD;

                        send(userToListen->userTCPSocket, vSendBuffer, 1, 0);

                        userToListen->lastTimeWrongPasswordEntered = std::chrono::steady_clock::now();
                    }
                }
            }
            else
            {
                memset(vSendBuffer, 0, MAX_NAME_LENGTH + 3);

                vSendBuffer[0] = RC_PASSWORD_REQ;
                vSendBuffer[1] = static_cast<char>(sRoomNameStr.size());

                std::memcpy(vSendBuffer + 2, sRoomNameStr.c_str(), sRoomNameStr.size());

                send(userToListen->userTCPSocket, vSendBuffer, 2 + static_cast<int>(sRoomNameStr.size()), 0);
            }
        }
    }

    mtxRooms.unlock();
}

void ServerService::userEntersRoom(UserStruct *userToListen, std::string sRoomName)
{
    char vSendBuffer[MAX_NAME_LENGTH + 3];
    memset(vSendBuffer, 0, MAX_NAME_LENGTH + 3);

    vSendBuffer[0] = RC_CAN_ENTER_ROOM;
    vSendBuffer[1] = static_cast<char>(sRoomName.size());

    std::memcpy(vSendBuffer + 2, sRoomName.c_str(), sRoomName.size());

    send(userToListen->userTCPSocket, vSendBuffer, 2 + static_cast<int>(sRoomName.size()), 0);

    pMainWindow->moveUserToRoom(userToListen->pUserInList, sRoomName);


    // Tell others.

    char vResendBuffer[MAX_NAME_LENGTH * 2 + 5];
    memset(vResendBuffer, 0, MAX_NAME_LENGTH * 2 + 5);

    size_t iIndex = 0;

    vResendBuffer[0] = RC_USER_ENTERS_ROOM;
    vResendBuffer[1] = static_cast<char>(userToListen->userName.size());

    iIndex = 2;

    std::memcpy(vResendBuffer + iIndex, userToListen->userName.c_str(), userToListen->userName.size());
    iIndex += userToListen->userName.size();

    vResendBuffer[iIndex] = static_cast<char>(sRoomName.size());
    iIndex += 1;

    std::memcpy(vResendBuffer + iIndex, sRoomName.c_str(), sRoomName.size());
    iIndex += sRoomName.size();

    for (size_t i = 0; i < users.size(); i++)
    {
        if (users[i] != userToListen)
        {
            int iSentSize = send(users[i]->userTCPSocket, vResendBuffer, static_cast<int>(iIndex), 0);

            if (iSentSize != static_cast<int>(iIndex))
            {
                if (iSentSize == SOCKET_ERROR)
                {
                    pLogManager ->printAndLog( "ServerService::checkRoomSettings::send() function failed and returned: "
                                               + std::to_string(getLastError()), true);
                }
                else
                {
                    pLogManager ->printAndLog( users[i]->userName + "'s info about room change wasn't fully sent. send() returned: "
                                               + std::to_string(iSentSize), true );
                }
            }
        }
    }
}

void ServerService::moveRoom(const std::string &sRoomName, bool bMoveUp)
{
    char vBuffer[2 + MAX_NAME_LENGTH + 2];
    memset(vBuffer, 0, 4 + MAX_NAME_LENGTH);

    vBuffer[0] = RC_SERVER_MOVED_ROOM;
    vBuffer[1] = static_cast<char>(sRoomName.size());

    std::memcpy(vBuffer + 2, sRoomName.c_str(), sRoomName.size());
    vBuffer[2 + sRoomName.size()] = bMoveUp;

    int iSizeToSend = 3 + static_cast<int>(sRoomName.size());
    int iSentSize   = 0;

    mtxUsersDelete.lock();
    mtxRooms.lock();

    for (size_t i = 0; i < users.size(); i++)
    {
        iSentSize = send(users[i]->userTCPSocket, vBuffer, iSizeToSend, 0);

        if (iSentSize != iSizeToSend)
        {
            if (iSentSize == SOCKET_ERROR)
            {
                pLogManager ->printAndLog( "ServerService::moveRoom::send() function failed and returned: "
                                           + std::to_string(getLastError()), true);
            }
            else
            {
                pLogManager ->printAndLog( "ServerService::moveRoom::send(): not full sent size on user " + users[i]->userName + ". send() returned: "
                                           + std::to_string(iSentSize), true );
            }
        }
    }

    mtxRooms.unlock();
    mtxUsersDelete.unlock();
}

void ServerService::deleteRoom(const std::string &sRoomName)
{
    std::vector<std::string> vRoomNames = pMainWindow->getRoomNames();

    char vBuffer[2 + MAX_NAME_LENGTH + 2];
    memset(vBuffer, 0, 4 + MAX_NAME_LENGTH);

    vBuffer[0] = RC_SERVER_DELETES_ROOM;
    vBuffer[1] = static_cast<char>(sRoomName.size());

    std::memcpy(vBuffer + 2, sRoomName.c_str(), sRoomName.size());

    int iSizeToSend = 2 + static_cast<int>(sRoomName.size());
    int iSentSize   = 0;


    for (size_t i = 0; i < vRoomNames.size(); i++)
    {
        if (vRoomNames[i] == sRoomName)
        {
            mtxRooms.lock();
            mtxUsersDelete.lock();

            if (pMainWindow->getUsersOfRoomIndex(i).size() == 0)
            {
                for (size_t i = 0; i < users.size(); i++)
                {
                    iSentSize = send(users[i]->userTCPSocket, vBuffer, iSizeToSend, 0);

                    if (iSentSize != iSizeToSend)
                    {
                        if (iSentSize == SOCKET_ERROR)
                        {
                            pLogManager ->printAndLog( "ServerService::deleteRoom::send() function failed and returned: "
                                                       + std::to_string(getLastError()), true);
                        }
                        else
                        {
                            pLogManager ->printAndLog( "ServerService::deleteRoom::send(): not full sent size on user " + users[i]->userName + ". send() returned: "
                                                       + std::to_string(iSentSize), true );
                        }
                    }
                }
            }
            else
            {
                pMainWindow->showMessageBox(true, u"Cannot delete the room because there are users in it.", true);
            }

            mtxUsersDelete.unlock();
            mtxRooms.unlock();

            break;
        }
    }
}

void ServerService::createRoom(const std::string &sName, size_t iMaxUsers)
{
    char vBuffer[MAX_NAME_LENGTH * 2];
    memset(vBuffer, 0, MAX_NAME_LENGTH * 2);

    vBuffer[0] = RC_SERVER_CREATES_ROOM;


    vBuffer[1] = static_cast<char>(sName.size());

    std::memcpy(vBuffer + 2, sName.c_str(), sName.size());

    int iIndex = 2 + static_cast<int>(sName.size());

    unsigned int iMax = static_cast<unsigned int>(iMaxUsers);

    std::memcpy(vBuffer + iIndex, &iMax, sizeof(unsigned int));
    iIndex += static_cast<int>(sizeof(unsigned int));


    int iSentSize   = 0;

    mtxUsersDelete.lock();
    mtxRooms.lock();

    for (size_t i = 0; i < users.size(); i++)
    {
        iSentSize = send(users[i]->userTCPSocket, vBuffer, iIndex, 0);

        if (iSentSize != iIndex)
        {
            if (iSentSize == SOCKET_ERROR)
            {
                pLogManager ->printAndLog( "ServerService::createRoom::send() function failed and returned: "
                                           + std::to_string(getLastError()), true);
            }
            else
            {
                pLogManager ->printAndLog( "ServerService::createRoom::send(): not full sent size on user " + users[i]->userName + ". send() returned: "
                                           + std::to_string(iSentSize), true );
            }
        }
    }

    mtxRooms.unlock();
    mtxUsersDelete.unlock();
}

void ServerService::changeRoomSettings(const std::string &sOldName, const std::string &sNewName, size_t iMaxUsers)
{
    char vBuffer[MAX_NAME_LENGTH * 3];
    memset(vBuffer, 0, MAX_NAME_LENGTH);

    vBuffer[0] = RC_SERVER_CHANGES_ROOM;

    vBuffer[1] = static_cast<char>(sOldName.size());

    int iIndex = 2;

    std::memcpy(vBuffer + iIndex, sOldName.c_str(), sOldName.size());
    iIndex += sOldName.size();

    vBuffer[iIndex] = static_cast<char>(sNewName.size());
    iIndex++;

    std::memcpy(vBuffer + iIndex, sNewName.c_str(), sNewName.size());
    iIndex += sNewName.size();

    unsigned int iMax = static_cast<unsigned int>(iMaxUsers);
    std::memcpy(vBuffer + iIndex, &iMax, sizeof(unsigned int));
    iIndex += sizeof(unsigned int);


    int iSentSize   = 0;

    mtxUsersDelete.lock();
    mtxRooms.lock();

    for (size_t i = 0; i < users.size(); i++)
    {
        iSentSize = send(users[i]->userTCPSocket, vBuffer, iIndex, 0);

        if (iSentSize != iIndex)
        {
            if (iSentSize == SOCKET_ERROR)
            {
                pLogManager ->printAndLog( "ServerService::changeRoomSettings::send() function failed and returned: "
                                           + std::to_string(getLastError()), true);
            }
            else
            {
                pLogManager ->printAndLog( "ServerService::changeRoomSettings::send(): not full sent size on user " + users[i]->userName + ". send() returned: "
                                           + std::to_string(iSentSize), true );
            }
        }
    }

    mtxRooms.unlock();
    mtxUsersDelete.unlock();
}

void ServerService::checkPing()
{
    while ( bVoiceListen && users.size() > 0 )
    {
        for (size_t i = 0; i < PING_CHECK_INTERVAL_SEC; i++)
        {
            std::this_thread::sleep_for(std::chrono::seconds(1));

            if (bVoiceListen == false) return;
        }

        int iSentSize = 0;

        char sendBuffer[sizeof(char)];
        sendBuffer[0] = UDP_SM_PING;

        mtxConnectDisconnect .lock();
        mtxUsersDelete .lock();

        pingCheckSendTime = std::chrono::steady_clock::now();

        for (unsigned int i = 0; i < users.size(); i++)
        {
            if ( users[i]->bConnectedToVOIP )
            {
                iSentSize = sendto(UDPsocket, sendBuffer, sizeof(char), 0,
                                   reinterpret_cast<sockaddr*>(&users[i]->userUDPAddr), sizeof(users[i]->userUDPAddr));

                if (iSentSize != sizeof(char))
                {
                    if (iSentSize == SOCKET_ERROR)
                    {
                        pLogManager ->printAndLog( "ServerService::checkPing::sendto() function failed and returned: "
                                                   + std::to_string(getLastError()), true);
                    }
                    else
                    {
                        pLogManager ->printAndLog( users[i]->userName + "'s ping check wasn't fully sent. sendto() returned: "
                                                   + std::to_string(iSentSize), true );
                    }
                }
            }
        }

        mtxUsersDelete .unlock();
        mtxConnectDisconnect .unlock();


        for (size_t i = 0; i < PING_ANSWER_WAIT_TIME_SEC; i++)
        {
            std::this_thread::sleep_for(std::chrono::seconds(1));

            if (bVoiceListen == false) return;
        }


        if (bVoiceListen) sendPingToAll();
    }
}

void ServerService::sendPingToAll(UserStruct* pNewUser)
{
    char sendBuffer[MAX_TCP_BUFFER_SIZE];

    sendBuffer[0] = SM_PING;
    // here (in index '1-2' we will insert packet size later)
    int iCurrentPos = 3;

    mtxConnectDisconnect .lock();
    mtxUsersDelete .lock();

    size_t iAllUsers = users .size();

    for (size_t i = 0; i < users.size(); i++)
    {
        // Check if we are able to add another user to 'sendBuffer'.

        if ( (iCurrentPos <= (MAX_BUFFER_SIZE - 1 - static_cast<int>(users[i]->userName.size())
                             - static_cast<int>(sizeof(users[i]->iPing))))
             &&
             (users[i]->bConnectedToVOIP) )
        {
            // Copy size of name
            sendBuffer[iCurrentPos] = static_cast<char>(users[i]->userName.size());
            iCurrentPos++;

            // Copy name
            memcpy(sendBuffer + iCurrentPos, users[i]->userName.c_str(), users[i]->userName.size());
            iCurrentPos += static_cast <int> (users[i]->userName.size());

            // Copy ping
            memcpy(sendBuffer + iCurrentPos, &users[i]->iPing, sizeof(users[i]->iPing));
            iCurrentPos += sizeof(users[i]->iPing);

            pMainWindow->setPingToUser(users[i]->pUserInList, users[i]->iPing);
        }
        else if (users[i]->bConnectedToVOIP == false)
        {
            iAllUsers--;
        }
        else
        {
            pLogManager ->printAndLog( "ServerService::sendPingToAll() sendBuffer is full. " + std::to_string(i)
                                       + "/" + std::to_string(iAllUsers) + " users was added to the buffer.", true );
            break;
        }
    }


    unsigned short iPacketSize = static_cast<unsigned short>(iCurrentPos - 3);

    // Insert packet size
    memcpy(sendBuffer + 1, &iPacketSize, sizeof(unsigned short));

    int iSentSize = 0;

    if (pNewUser)
    {
        // Send this packet (about all users) to new user.

        iSentSize = send(pNewUser->userTCPSocket, sendBuffer, iCurrentPos, 0);

        if ( iSentSize != iCurrentPos )
        {
            if (iSentSize == SOCKET_ERROR)
            {
                pLogManager ->printAndLog( "ServerService::sendPingToAll::send() function failed and returned: "
                                           + std::to_string(getLastError()), true );
            }
            else
            {
                pLogManager ->printAndLog( pNewUser->userName + "'s ping report wasn't fully sent. send() returned: "
                                           + std::to_string(iSentSize), true );
            }
        }


        // And send this new user's ping to all.
        char vSingleUserPingBuffer[MAX_BUFFER_SIZE];

        vSingleUserPingBuffer[0] = SM_PING;
        // here (in index '1-2' we will insert packet size later)
        int iCurrentSignlePos = 3;


        // Copy size of name
        vSingleUserPingBuffer[iCurrentSignlePos] = static_cast<char>(pNewUser->userName.size());
        iCurrentSignlePos++;

        // Copy name
        memcpy(vSingleUserPingBuffer + iCurrentSignlePos, pNewUser->userName.c_str(), pNewUser->userName.size());
        iCurrentSignlePos += static_cast <int> (pNewUser->userName.size());

        // Copy ping
        memcpy(vSingleUserPingBuffer + iCurrentSignlePos, &pNewUser->iPing, sizeof(pNewUser->iPing));
        iCurrentSignlePos += sizeof(pNewUser->iPing);

        unsigned short iSinglePacketSize = static_cast<unsigned short>(iCurrentSignlePos - 3);

        // Insert packet size
        memcpy(vSingleUserPingBuffer + 1, &iSinglePacketSize, sizeof(unsigned short));


        int iSingleSentSize = 0;

        for (size_t i = 0; i < users.size(); i++)
        {
            iSingleSentSize = send(users[i]->userTCPSocket, vSingleUserPingBuffer, iCurrentSignlePos, 0);
            if ( iSingleSentSize != iCurrentSignlePos )
            {
                if (iSingleSentSize == SOCKET_ERROR)
                {
                    pLogManager ->printAndLog( "ServerService::sendPingToAll::send() function failed and returned: "
                                               + std::to_string(getLastError()), true );
                }
                else
                {
                    pLogManager ->printAndLog( users[i]->userName + "'s ping report wasn't fully sent. send() returned: "
                                               + std::to_string(iSentSize), true );
                }
            }
        }
    }
    else
    {
        // Send to all.

        for (size_t i = 0; i < users.size(); i++)
        {
            iSentSize = send(users[i]->userTCPSocket, sendBuffer, iCurrentPos, 0);
            if ( iSentSize != iCurrentPos )
            {
                if (iSentSize == SOCKET_ERROR)
                {
                    pLogManager ->printAndLog( "ServerService::sendPingToAll::send() function failed and returned: "
                                               + std::to_string(getLastError()), true );
                }
                else
                {
                    pLogManager ->printAndLog( users[i]->userName + "'s ping report wasn't fully sent. send() returned: "
                                               + std::to_string(iSentSize), true );
                }
            }
        }
    }


    mtxUsersDelete .unlock();
    mtxConnectDisconnect .unlock();
}

void ServerService::responseToFIN(UserStruct* userToClose, bool bUserLost)
{
    mtxConnectDisconnect .lock();
    mtxUsersDelete .lock();

    userToClose ->bConnectedToVOIP     = false;
    userToClose ->bConnectedToTextChat = false;
    iUsersConnectedToVOIP--;

    mtxUsersDelete .unlock();


    if (userToClose->bConnectedToVOIP)
    {
        // Wait for listenForVoiceMessage() to end.
        std::this_thread::sleep_for( std::chrono::milliseconds(INTERVAL_UDP_MESSAGE_MS) );

        mtxUDPPackets .lock();

        for (int i = 0; i < static_cast <int>(vUDPPackets .size()); i++)
        {
#if _WIN32
            if (    (vUDPPackets[i] ->senderInfo.sin_addr.S_un.S_un_b.s_b1 == userToClose->userUDPAddr.sin_addr.S_un.S_un_b.s_b1)
                 && (vUDPPackets[i] ->senderInfo.sin_addr.S_un.S_un_b.s_b2 == userToClose->userUDPAddr.sin_addr.S_un.S_un_b.s_b2)
                 && (vUDPPackets[i] ->senderInfo.sin_addr.S_un.S_un_b.s_b3 == userToClose->userUDPAddr.sin_addr.S_un.S_un_b.s_b3)
                 && (vUDPPackets[i] ->senderInfo.sin_addr.S_un.S_un_b.s_b4 == userToClose->userUDPAddr.sin_addr.S_un.S_un_b.s_b4)
                 && (userToClose ->userUDPAddr.sin_port == vUDPPackets[i] ->senderInfo.sin_port) )
            {
#elif __linux__
            if (    (vUDPPackets[i] ->senderInfo.sin_addr.s_addr == userToClose->userUDPAddr.sin_addr.s_addr)
                 && (userToClose ->userUDPAddr.sin_port == vUDPPackets[i] ->senderInfo.sin_port) )
            {
#endif
                delete vUDPPackets[i];
                vUDPPackets .erase( vUDPPackets .begin() + i );
                i--;
            }
        }

        mtxUDPPackets .unlock();
    }

    //mtxConnectDisconnect .unlock();

    // Wait for listenForMessage() to end.
    //std::this_thread::sleep_for( std::chrono::milliseconds(INTERVAL_TCP_MESSAGE_MS) );
    std::this_thread::sleep_for( std::chrono::milliseconds(25) ); // 25 because look end of listenForMessage().

    //mtxConnectDisconnect .lock();


    iUsersConnectedCount--;
    pMainWindow ->updateOnlineUsersCount(iUsersConnectedCount);


    // Clear 'users' massive

    if ( (users .size() - 1) != 0)
    {
        // Tell other users that one is disconnected
        char sendBuffer[MAX_NAME_LENGTH + 3 + 5];
        memset(sendBuffer, 0, MAX_NAME_LENGTH + 3 + 5);


        sendBuffer[0] = SM_SOMEONE_DISCONNECTED;

        if (bUserLost)
        {
            sendBuffer[1] = UDR_LOST;
        }
        else
        {
            sendBuffer[1] = UDR_DISCONNECT;
        }


        unsigned char iPacketSize = static_cast <unsigned char> (7 + userToClose->userName.size());

        memcpy(sendBuffer + 2, &iPacketSize, 1);

        memcpy(sendBuffer + 3, &iUsersConnectedCount, 4);

        memcpy(sendBuffer + 7, userToClose->userName.c_str(), userToClose->userName.size());

        for (size_t j = 0; j < users.size(); j++)
        {
            if ( users[j] ->userName != userToClose ->userName )
            {
               send(users[j]->userTCPSocket, sendBuffer, iPacketSize, 0);
            }
        }
    }



    if (!bUserLost)
    {
        // Client sent FIN
        // We are responding:
        pLogManager ->printAndLog( userToClose ->userName + " has sent FIN.", true );

        int returnCode = shutdown(userToClose->userTCPSocket, SD_SEND);

        if (returnCode == SOCKET_ERROR)
        {
            pLogManager ->printAndLog( "ServerService::responseToFIN()::shutdown() function failed and returned: "
                                       + std::to_string(getLastError()) + ".", true );

            returnCode = shutdown(userToClose->userTCPSocket, SD_SEND);

            if (returnCode == SOCKET_ERROR)
            {
                pLogManager ->printAndLog( "Try #2. Can't shutdown socket. Closing socket...", true );

                if (closeSocket(userToClose->userTCPSocket))
                {
                    pLogManager ->printAndLog( "ServerService::responseToFIN()::closeSocket() function failed and returned: "
                                               + std::to_string(getLastError())
                                               + ".\n Can't even close this socket... You better reboot server.\n", true );
                }
            }
            else
            {
                pLogManager ->printAndLog( "Try #2. Shutdown success.", true );

                if (closeSocket(userToClose->userTCPSocket))
                {
                    pLogManager ->printAndLog( "ServerService::responseToFIN()::closeSocket() function failed and returned: "
                                               + std::to_string(getLastError())
                                               + ".\n Can't even close this socket... You better reboot server.\n", true );
                }
                else
                {
                    pLogManager ->printAndLog( "Successfully closed the socket.\n", true );
                }
            }
        }
        else
        {
            if (closeSocket(userToClose->userTCPSocket))
            {
                pLogManager ->printAndLog( "ServerService::responseToFIN()::closeSocket() function failed and returned: "
                                           + std::to_string(getLastError())
                                           + ".\n Can't even close this socket... You better reboot server.\n", true );
            }
            else
            {
                pLogManager ->printAndLog( "Successfully closed connection with " + userToClose->userName + ".\n", true );
            }
        }
    }
    else
    {
        pLogManager ->printAndLog( "Lost connection with " + userToClose->userName + ". Closing socket...\n", true );
        closeSocket(userToClose->userTCPSocket);
    }



    mtxUsersDelete .lock();


    // Erase user from vector.

    for (unsigned int i = 0;   i < users .size();   i++)
    {
        if (users[i] ->userName == userToClose ->userName)
        {
            delete[] userToClose ->pDataFromUser;
            pMainWindow ->deleteUserFromList( userToClose ->pUserInList );

            delete users[i];
            users .erase( users .begin() + i );

            break;
        }
    }


    mtxUsersDelete .unlock();

    mtxConnectDisconnect .unlock();
}

void ServerService::kickUser(SListItemUser *pUser)
{
    for (size_t i = 0; i < users .size(); i++)
    {
        if ( users[i] ->pUserInList == pUser )
        {
            std::thread tKickThread(&ServerService::sendFINtoUser, this, users[i]);
            tKickThread .detach();

            break;
        }
    }
}

void ServerService::sendFINtoUser(UserStruct *userToClose)
{
    char cKickedNotice = SM_KICKED;

    int iSentSize = send( userToClose ->userTCPSocket, &cKickedNotice, sizeof(cKickedNotice), 0 );

    if (iSentSize != sizeof(cKickedNotice))
    {
        pLogManager ->printAndLog( "Can't send the \"kick notice\" to the user \"" + userToClose ->userName
                                   + "\". Closing connection.");
    }


    pLogManager ->printAndLog("Kicked the user \"" + userToClose ->userName + "\" from the server.\n");


    mtxConnectDisconnect .lock();

    if (userToClose->bConnectedToVOIP)
    {
        mtxUsersDelete .lock();

        userToClose->bConnectedToVOIP     = false;
        userToClose->bConnectedToTextChat = false;
        iUsersConnectedToVOIP--;

        mtxUsersDelete .unlock();

        // Wait for listenForVoiceMessage() and listenForMessage() to end.
        std::this_thread::sleep_for( std::chrono::milliseconds(INTERVAL_UDP_MESSAGE_MS) );
        std::this_thread::sleep_for( std::chrono::milliseconds(INTERVAL_TCP_MESSAGE_MS) );

        mtxUDPPackets .lock();


        // This never happened (I think) but let's check if there are some packets from this user. Just in case.

        for (int i = 0; i < vUDPPackets .size(); i++)
        {
#if _WIN32
            if (    (vUDPPackets[i] ->senderInfo.sin_addr.S_un.S_un_b.s_b1 == userToClose->userUDPAddr.sin_addr.S_un.S_un_b.s_b1)
                 && (vUDPPackets[i] ->senderInfo.sin_addr.S_un.S_un_b.s_b2 == userToClose->userUDPAddr.sin_addr.S_un.S_un_b.s_b2)
                 && (vUDPPackets[i] ->senderInfo.sin_addr.S_un.S_un_b.s_b3 == userToClose->userUDPAddr.sin_addr.S_un.S_un_b.s_b3)
                 && (vUDPPackets[i] ->senderInfo.sin_addr.S_un.S_un_b.s_b4 == userToClose->userUDPAddr.sin_addr.S_un.S_un_b.s_b4)
                 && (userToClose ->userUDPAddr.sin_port == vUDPPackets[i] ->senderInfo.sin_port) )
            {
#elif __linux__
            if (    (vUDPPackets[i] ->senderInfo.sin_addr.s_addr == userToClose->userUDPAddr.sin_addr.s_addr)
                 && (userToClose ->userUDPAddr.sin_port == vUDPPackets[i] ->senderInfo.sin_port) )
            {
#endif
                delete vUDPPackets[i];
                vUDPPackets .erase( vUDPPackets .begin() + i );
                i--;
            }
        }

        mtxUDPPackets .unlock();
    }


    sendFINtoSocket(userToClose ->userTCPSocket);


    iUsersConnectedCount--;
    pMainWindow->updateOnlineUsersCount(iUsersConnectedCount);


    // Clear 'users' massive

    if (users.size() - 1 != 0)
    {
        // Tell other users that one is disconnected
        char sendBuffer[MAX_NAME_LENGTH + 3 + 5];
        memset(sendBuffer, 0, MAX_NAME_LENGTH + 3 + 5);


        sendBuffer[0] = SM_SOMEONE_DISCONNECTED;
        sendBuffer[1] = UDR_KICKED;


        unsigned char iPacketSize = static_cast <unsigned char> (7 + userToClose->userName.size());

        memcpy(sendBuffer + 2, &iPacketSize, 1);

        memcpy(sendBuffer + 3, &iUsersConnectedCount, 4);

        memcpy(sendBuffer + 7, userToClose->userName.c_str(), userToClose->userName.size());

        for (size_t j = 0; j < users.size(); j++)
        {
            if ( users[j] ->userName != userToClose ->userName )
            {
               send(users[j]->userTCPSocket, sendBuffer, iPacketSize, 0);
            }
        }
    }


    mtxUsersDelete .lock();



    // Erase user from massive
    for (unsigned int i = 0; i < users.size(); i++)
    {
        if (users[i]->userName == userToClose->userName)
        {
            users.erase(users.begin() + i);

            break;
        }
    }

    pMainWindow->deleteUserFromList(userToClose->pUserInList);
    delete[] userToClose->pDataFromUser;
    delete userToClose;


    mtxUsersDelete .unlock();


    mtxConnectDisconnect .unlock();
}

void ServerService::sendFINtoSocket(SSocket socketToClose)
{
    // Translate socket to blocking mode

    setSocketBlocking(socketToClose, true);

    int returnCode = shutdown(socketToClose, SD_SEND);
    if (returnCode == SOCKET_ERROR)
    {
        pLogManager ->printAndLog( "ServerService::sendFINtoSocket()::shutdown() function failed and returned: "
                                   + std::to_string(getLastError()) + ".", true );
        closeSocket(socketToClose);
    }
    else
    {
        char tempBuffer[5];
        returnCode = recv(socketToClose, tempBuffer, 5, 0);
        if (returnCode == 0)
        {
            if (closeSocket(socketToClose))
            {
                pLogManager ->printAndLog( "ServerService::sendFINtoSocket()::closeSocket() function failed and returned: "
                                           + std::to_string(getLastError())
                                           + ".\nShutdown done but can't close socket... You better reboot the server.\n", true);
            }
            else
            {
                pLogManager ->printAndLog( "Received FIN and closed socket.", true );
            }
        }
        else
        {
            returnCode = recv(socketToClose, tempBuffer, 5, 0);
            if (returnCode == 0)
            {
                if (closeSocket(socketToClose))
                {
                    pLogManager ->printAndLog( "ServerService::sendFINtoSocket()::closeSocket() function failed and returned: "
                                               + std::to_string(getLastError())
                                               + ".\nShutdown done but can't close socket... You better reboot the server.\n", true);

                }
                else
                {
                    pLogManager ->printAndLog( "Try #2. Received FIN and closed socket.", true );
                }
            }
        }
    }
}

void ServerService::shutdownAllUsers()
{
    if (users.size() != 0)
    {
        mtxConnectDisconnect .lock();

        pLogManager ->printAndLog( "Shutting down...\n");

        pMainWindow->updateOnlineUsersCount(0);

        mtxUsersDelete .lock();

        for (unsigned int i = 0; i < users.size(); i++)
        {
            if (users[i]->bConnectedToVOIP)
            {
                users[i]->bConnectedToVOIP = false;
            }
            users[i]->bConnectedToTextChat = false;
        }

        mtxUsersDelete .unlock();

        // Now we will not listen for new sockets and we also
        // will not listen connected users (because in listenForNewMessage function while cycle will fail)
        bTextListen = false;
        bVoiceListen = false;

        std::this_thread::sleep_for(std::chrono::milliseconds(INTERVAL_TCP_ACCEPT_MS));

        closeSocket(listenTCPSocket);
        closeSocket(UDPsocket);

        iUsersConnectedToVOIP = 0;



        int correctlyClosedSocketsCount = 0;
        int socketsCount                = static_cast<int>(users.size());

        int returnCode;

        // We send FIN to all
        for (size_t i = 0; i < users.size(); i++)
        {
            returnCode = shutdown(users[i]->userTCPSocket, SD_SEND);
            if (returnCode == SOCKET_ERROR)
            {
                pLogManager ->printAndLog( "ServerService::shutdownAllUsers()::shutdown() function failed and returned: "
                                           + std::to_string(getLastError()) + ". Just closing it...");
                closeSocket(users[i]->userTCPSocket);

                // Delete user's read buffer & delete him from the list
                delete[] users[i]->pDataFromUser;

                pMainWindow->deleteUserFromList(users[i]->pUserInList);
                delete users[i];

                users.erase(users.begin() + static_cast<long long>(i));
                i--;
            }
        }


        // Translate all sockets to blocking mode
        for (size_t i = 0; i < users.size(); i++)
        {
            // Socket isn't closed
            if (setSocketBlocking(users[i]->userTCPSocket, true))
            {
                pLogManager ->printAndLog( "ServerService::shutdownAllUsers()::ioctsocket() (set blocking mode) failed and returned: "
                                           + std::to_string(getLastError()) + ". Just closing it...");
                closeSocket(users[i]->userTCPSocket);

                // Delete user's read buffer & delete him from list
                delete[] users[i]->pDataFromUser;

                pMainWindow->deleteUserFromList(users[i]->pUserInList);
                delete users[i];

                users.erase(users.begin() + static_cast<long long>(i));
                i--;
            }
        }

        bool tryAgainToClose = false;
        pLogManager ->printAndLog( "Sent FIN packets to all users. Waiting for the response...");

        // We are waiting for a response
        for (size_t i = 0; i < users.size(); i++)
        {
            // Socket isn't closed
            returnCode = recv(users[i]->userTCPSocket, users[i]->pDataFromUser, MAX_BUFFER_SIZE, 0);
            if (returnCode == 0)
            {
                // FIN received
                if (closeSocket(users[i]->userTCPSocket))
                {
                    pLogManager ->printAndLog( "ServerService::shutdownAllUsers()::closeSocket() function failed and returned: "
                                               + std::to_string(getLastError()) + "." );
                }
                else
                {
                    correctlyClosedSocketsCount++;
                }

                // Delete user's read buffer & delete him from list
                delete[] users[i]->pDataFromUser;

                pMainWindow->deleteUserFromList(users[i]->pUserInList);
                delete users[i];

                users.erase(users.begin() + static_cast<long long>(i));
                i--;
            }
            else
            {
                tryAgainToClose = true;
            }
        }

        if (tryAgainToClose)
        {
            // Try again to close the sockets that does not returned FIN
            for (size_t i = 0; i < users.size(); i++)
            {
                // Socket isn't closed
                returnCode = recv(users[i]->userTCPSocket,users[i]->pDataFromUser, MAX_BUFFER_SIZE, 0);
                if (returnCode == 0)
                {
                    // FIN received
                    if (closeSocket(users[i]->userTCPSocket))
                    {
                        pLogManager ->printAndLog( "ServerService::shutdownAllUsers()::closeSocket() function failed and returned: "
                                                   + std::to_string(getLastError()) + "." );
                    }
                    else
                    {
                        correctlyClosedSocketsCount++;
                    }
                }
                else
                {
                    pLogManager ->printAndLog( "FIN wasn't received from client for the second time... Closing socket..." );
                }

                // Delete user's read buffer & delete him from list
                delete[] users[i]->pDataFromUser;

                pMainWindow->deleteUserFromList(users[i]->pUserInList);
                delete users[i];

                users.erase(users.begin() + static_cast<long long>(i));
                i--;
            }
        }

        pLogManager ->printAndLog( "Correctly closed sockets: " + std::to_string(correctlyClosedSocketsCount)
                                   + "/" + std::to_string(socketsCount) + "." );

        // Clear users massive if someone is still there
        for (size_t i = 0; i < users.size(); i++)
        {
            delete[] users[i]->pDataFromUser;

            pMainWindow->deleteUserFromList(users[i]->pUserInList);
            delete users[i];
        }

        users.clear();


        iUsersConnectedCount = 0;


        if (bWinSockStarted)
        {
#ifdef _WIN32
            if (WSACleanup() == SOCKET_ERROR)
            {
                pLogManager ->printAndLog( "ServerService::shutdownAllUsers()::WSACleanup() function failed and returned: "
                                           + std::to_string(getLastError()) + "." );
            }
#endif

            bWinSockStarted = false;
        }

        mtxConnectDisconnect .unlock();
    }
    else
    {
        // Now we will not listen for new sockets and we also
        // will not listen connected users (because in listenForNewMessage function while cycle will fail)
        bTextListen = false;
        bVoiceListen = false;

        closeSocket(listenTCPSocket);
        closeSocket(UDPsocket);
    }


    mtxUDPPackets .lock();
    mtxUDPPackets .unlock();

    for (size_t i = 0; i < vUDPPackets .size(); i++)
    {
        delete vUDPPackets[i];
    }

    vUDPPackets .clear();


    pLogManager ->printAndLog( "Server stopped.\n" );
    pMainWindow->changeStartStopActionText(false);
    pMainWindow->showSendMessageToAllAction(false);
}
