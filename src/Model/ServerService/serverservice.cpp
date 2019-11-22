#include "serverservice.h"

//STL
#include <thread>

//Custom
#include "../src/View/MainWindow/mainwindow.h"
#include "../src/Model/net_params.h"
#include "../src/Model/SettingsManager/settingsmanager.h"
#include "../src/Model/SettingsManager/SettingsFile.h"


ServerService::ServerService(MainWindow* pMainWindow, SettingsManager* pSettingsManager)
{
    this ->pMainWindow         = pMainWindow;
    this ->pSettingsManager    = pSettingsManager;


    // should be shorter than MAX_VERSION_STRING_LENGTH
    serverVersion              = SERVER_VERSION;
    clientLastSupportedVersion = CLIENT_SUPPORTED_VERSION;


    iPingNormalBelow           = PING_NORMAL_BELOW;
    iPingWarningBelow          = PING_WARNING_BELOW;


    iUsersConnectedCount       = 0;
    iUsersConnectedToVOIP      = 0;


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

bool ServerService::startWinSock()
{
    pMainWindow->clearChatWindow();
    pMainWindow->printOutput( std::string("Starting...") );

    // Start Winsock2

    WSADATA WSAData;
    // Start WinSock2 (ver. 2.2)
    if (WSAStartup(MAKEWORD(2, 2), &WSAData) != 0)
    {
        pMainWindow->printOutput(std::string("WSAStartup function failed and returned: " + std::to_string(WSAGetLastError()) + ".\nTry again.\n"));
    }
    else
    {
        bWinSockStarted = true;

        startToListenForConnection();
        if (bTextListen)
        {
            pMainWindow->changeStartStopActionText(true);

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
        pMainWindow->printOutput(std::string("ServerService::listenForConnection()::socket() function failed and returned: " + std::to_string(WSAGetLastError()) + "."));
    }
    else
    {
        // Create and fill the "sockaddr_in" structure containing the IPv4 socket
        sockaddr_in myAddr;
        memset(myAddr.sin_zero, 0, sizeof(myAddr.sin_zero));
        myAddr .sin_family      = AF_INET;
        myAddr .sin_port        = htons( pSettingsManager ->getCurrentSettings() ->iPort );
        myAddr .sin_addr.s_addr = htonl(INADDR_ANY);

        if (bind(listenTCPSocket, reinterpret_cast<sockaddr*>(&myAddr), sizeof(myAddr)) == SOCKET_ERROR)
        {
            pMainWindow->printOutput(std::string("ServerService::listenForConnection()::bind() function failed and returned: " + std::to_string(WSAGetLastError()) + ".\nSocket will be closed. Try again.\n"));
            closesocket(listenTCPSocket);
        }
        else
        {
            // Find out local port and show it
            sockaddr_in myBindedAddr;
            int len = sizeof(myBindedAddr);
            getsockname(listenTCPSocket, reinterpret_cast<sockaddr*>(&myBindedAddr), &len);

            // Get my IP
            char myIP[16];
            inet_ntop(AF_INET, &myBindedAddr.sin_addr, myIP, sizeof(myIP));

            pMainWindow->printOutput(std::string("Success. Waiting for a connection requests on port: " + std::to_string(ntohs(myBindedAddr.sin_port)) + "."));

            if (listen(listenTCPSocket, SOMAXCONN) == SOCKET_ERROR)
            {
                pMainWindow->printOutput(std::string("ServerService::listenForConnection()::listen() function failed and returned: " + std::to_string(WSAGetLastError()) + ".\nSocket will be closed. Try again.\n"));
                closesocket(listenTCPSocket);
            }
            else
            {
                pMainWindow->printOutput(std::string("\nWARNING:\nThe data transmitted over the network is not encrypted.\n"));

                // Translate listen socket to non-blocking mode
                u_long arg = true;
                if (ioctlsocket(listenTCPSocket, static_cast<long>(FIONBIO), &arg) == SOCKET_ERROR)
                {
                    pMainWindow->printOutput(std::string("ServerService::listenForConnection()::ioctsocket() failed and returned: " + std::to_string(WSAGetLastError()) + ".\nSocket will be closed. Try again.\n"));
                    closesocket(listenTCPSocket);
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
    int iLen = sizeof(connectedWith);

    // Accept new connection
    while (bTextListen)
    {
        SOCKET newConnectedSocket;
        newConnectedSocket = accept(listenTCPSocket, reinterpret_cast<sockaddr*>(&connectedWith), &iLen);
        while (newConnectedSocket != INVALID_SOCKET)
        {
            pMainWindow->printOutput(std::string("\nSomeone is connecting..."), true);
            // Disable Nagle algorithm for Connected Socket
            BOOL bOptVal = true;
            int bOptLen = sizeof(BOOL);
            if (setsockopt(newConnectedSocket, IPPROTO_TCP, TCP_NODELAY, reinterpret_cast<char*>(&bOptVal), bOptLen) == SOCKET_ERROR)
            {
                pMainWindow->printOutput(std::string("ServerService::listenForNewConnections()::setsockopt() (Nagle algorithm) failed and returned: " + std::to_string(WSAGetLastError()) + ".\nSending FIN to this new user.\n"),true);

                std::thread closethread(&ServerService::sendFINtoSocket, this, newConnectedSocket);
                closethread.detach();
            }
            else
            {
                // Receive version and user name
                char nameBuffer[MAX_NAME_LENGTH + MAX_VERSION_STRING_LENGTH + 1];
                memset(nameBuffer, 0, MAX_NAME_LENGTH + MAX_VERSION_STRING_LENGTH + 1);
                if (recv(newConnectedSocket, nameBuffer, MAX_NAME_LENGTH + MAX_VERSION_STRING_LENGTH + 1, 0) > 1)
                {
                    // Received version & user name

                    // Check if client version is the same with the server version
                    char clientVersionSize = nameBuffer[0];
                    char* pVersion = new char[ static_cast<size_t>(clientVersionSize + 1) ];
                    memset( pVersion, 0, static_cast<size_t>(clientVersionSize + 1) );

                    std::memcpy(pVersion, nameBuffer + 1, static_cast<size_t>(clientVersionSize));

                    std::string clientVersion(pVersion);
                    delete[] pVersion;
                    if ( clientVersion != clientLastSupportedVersion )
                    {
                        pMainWindow->printOutput(std::string("Client version " + clientVersion + " does not match with the last supported client version " + clientLastSupportedVersion + ".\n"
                                                             "Server version: " + serverVersion + "."), true);
                        char answerBuffer[MAX_VERSION_STRING_LENGTH + 2];
                        memset(answerBuffer, 0, MAX_VERSION_STRING_LENGTH + 2);

                        answerBuffer[0] = 3;
                        answerBuffer[1] = static_cast<char>(clientLastSupportedVersion.size());
                        std::memcpy(answerBuffer + 2, clientLastSupportedVersion.c_str(), clientLastSupportedVersion.size());

                        send(newConnectedSocket, answerBuffer, static_cast<int>(2 + clientLastSupportedVersion.size()), 0);
                        std::thread closethread(&ServerService::sendFINtoSocket, this, newConnectedSocket);
                        closethread.detach();

                        continue;
                    }

                    // Check if this user name is free
                    std::string userNameStr(nameBuffer + 1 + clientVersionSize);
                    bool bUserNameFree = true;
                    for (unsigned int i = 0; i<users.size(); i++)
                    {
                        if (users[i]->userName == userNameStr)
                        {
                            bUserNameFree = false;
                            break;
                        }
                    }

                    if (bUserNameFree == false)
                    {
                        pMainWindow->printOutput(std::string("User name " + userNameStr + " is already taken."),true);
                        char command = 0;
                        send(newConnectedSocket,reinterpret_cast<char*>(&command), 1, 0);
                        std::thread closethread(&ServerService::sendFINtoSocket, this, newConnectedSocket);
                        closethread.detach();
                    }
                    else
                    {
                        // Show with whom connected
                        char connectedWithIP[16];
                        memset(&connectedWithIP,0,16);
                        inet_ntop(AF_INET, &connectedWith.sin_addr, connectedWithIP, sizeof(connectedWithIP));

                        char tempData[MAX_BUFFER_SIZE];
                        memset(tempData, 0, MAX_BUFFER_SIZE);

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
                        char command = 4;
                        std::memcpy(tempData, &command, 1);
                        iBytesWillSend++;

                        // We will put here packet size
                        iBytesWillSend += 2;

                        std::memcpy(tempData + iBytesWillSend, &iUsersConnectedCount, 4);
                        iBytesWillSend += 4;
                        for (unsigned int j = 0; j < users.size(); j++)
                        {
                            unsigned char nameSize = static_cast<unsigned char>(users[j]->userName.size()) + 1;
                            std::memcpy(tempData + iBytesWillSend, &nameSize, 1);
                            iBytesWillSend++;

                            std::memcpy(tempData + iBytesWillSend, users[j]->userName.c_str(), nameSize);
                            iBytesWillSend += nameSize;
                        }

                        // Put packet size to buffer (packet size - command size (1 byte) - packet size (2 bytes))
                        unsigned short int iPacketSize = static_cast<unsigned short>(iBytesWillSend - 3);
                        std::memcpy(tempData + 1, &iPacketSize, 2);

                        if (iBytesWillSend > MAX_BUFFER_SIZE)
                        {
                            // This should happen when you got like >50 users online (when all users have name long 20 chars) if my calculations are correct.
                            // Not the main problem right now, tell me if you are suffering from this lol.

                            pMainWindow->printOutput(std::string("Server is full.\n"), true);

                            char serverIsFullCommand = 2;
                            send(newConnectedSocket,reinterpret_cast<char*>(&serverIsFullCommand), 1, 0);
                            std::thread closethread(&ServerService::sendFINtoSocket, this, newConnectedSocket);
                            closethread.detach();

                            continue;
                        }

                        // SEND
                        int iBytesWereSent = send(newConnectedSocket, tempData, iBytesWillSend, 0);

                        if (iBytesWereSent != iBytesWillSend)
                        {
                            pMainWindow->printOutput(std::string("\nWARNING:\n" + std::to_string(iBytesWereSent)+" bytes were sent of total " + std::to_string(iBytesWillSend) + " to new user.\n"), true);
                        }
                        if (iBytesWereSent == -1)
                        {
                            pMainWindow->printOutput(std::string("ServerService::listenForNewConnections()::send()) (online info) failed and returned: " + std::to_string(WSAGetLastError())+"."), true);
                            if (recv(newConnectedSocket, tempData, MAX_BUFFER_SIZE, 0) == 0)
                            {
                                pMainWindow->printOutput(std::string("received FIN from this new user who didn't receive online info."), true);
                                shutdown(newConnectedSocket, SD_SEND);
                                if (closesocket(newConnectedSocket) != SOCKET_ERROR)
                                {
                                    pMainWindow->printOutput(std::string("closed this socket with success."), true);
                                }
                                else
                                {
                                    pMainWindow->printOutput(std::string("can't close this socket... meh. You better reboot the server..."), true);
                                }
                            }
                            iUsersConnectedCount--;
                        }
                        else
                        {
                            // Translate new connected socket to non-blocking mode
                            u_long arg = true;
                            if (ioctlsocket(newConnectedSocket, static_cast<long>(FIONBIO), &arg) == SOCKET_ERROR)
                            {
                                pMainWindow->printOutput(std::string("ServerService::listenForNewConnections()::ioctsocket() (non-blocking mode) failed and returned: " + std::to_string(WSAGetLastError()) + "."), true);

                                std::thread closethread(&ServerService::sendFINtoSocket, this, newConnectedSocket);
                                closethread.detach();
                            }
                            else
                            {
                                if (users.size() != 0)
                                {
                                    // Tell other users about new user

                                    char newUserInfo[MAX_NAME_LENGTH + 11];
                                    memset(newUserInfo, 0, MAX_NAME_LENGTH + 11);

                                    unsigned char sizeOfUserName = static_cast<unsigned char>(userNameStr.size());

                                    unsigned char iSendSize = 0;

                                    // 0 - means 'there is new user, update your OnlineCount and add him to list
                                    unsigned char commandType = 0;
                                    std::memcpy(newUserInfo, &commandType, 1);
                                    iSendSize++;

                                    // Put packet size
                                    unsigned char iPacketSize = 4 + 1 + sizeOfUserName;
                                    std::memcpy(newUserInfo + 1, &iPacketSize, 1);
                                    iSendSize++;

                                    std::memcpy(newUserInfo + 2, &iUsersConnectedCount, 4);
                                    iSendSize += 4;

                                    std::memcpy(newUserInfo + 6, &sizeOfUserName, 1);
                                    iSendSize++;
                                    std::memcpy(newUserInfo + 7, userNameStr.c_str(), sizeOfUserName);
                                    iSendSize += sizeOfUserName;

                                    // Send this data
                                    for (unsigned int i = 0; i < users.size(); i++)
                                    {
                                        if ( send(users[i]->userTCPSocket, newUserInfo, iSendSize, 0) != iSendSize)
                                        {
                                            pMainWindow->printOutput( std::string("ServerService::listenForNewTCPConnections::send() failed (info about new user)."), true );
                                        }
                                    }
                                }


                                // Fill UserStruct for new user

                                UserStruct* pNewUser = new UserStruct();
                                pNewUser->userTCPSocket  = newConnectedSocket;
                                pNewUser->userName       = userNameStr;
                                pNewUser->pDataFromUser  = new char[MAX_BUFFER_SIZE];
                                pNewUser->userIP         = std::string(connectedWithIP);
                                pNewUser->userTCPPort    = ntohs(connectedWith.sin_port);
                                pNewUser->keepAliveTimer = clock();

                                memset(pNewUser->userUDPAddr.sin_zero, 0, sizeof(pNewUser->userUDPAddr.sin_zero));
                                pNewUser->userUDPAddr.sin_family = AF_INET;
                                pNewUser->userUDPAddr.sin_addr = connectedWith.sin_addr;
                                pNewUser->userUDPAddr.sin_port = 0;
                                pNewUser->iCurrentPing = 0;

                                pNewUser->bConnectedToTextChat = false;
                                pNewUser->bConnectedToVOIP = false;

                                users.push_back(pNewUser);

                                // Ready to send and receive data

                                pMainWindow->printOutput(std::string("Connected with " + std::string(connectedWithIP) + ":" + std::to_string(ntohs(connectedWith.sin_port)) + " AKA " + users.back()->userName + "."),true);
                                pMainWindow->updateOnlineUsersCount(iUsersConnectedCount);
                                users.back()->pListItem = pMainWindow->addNewUserToList(users.back()->userName);

                                std::thread listenThread(&ServerService::listenForMessage, this, users.back());
                                listenThread.detach();

                            }
                        }
                    }
                }
            }

            newConnectedSocket = accept(listenTCPSocket, reinterpret_cast<sockaddr*>(&connectedWith), &iLen);
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(INTERVAL_TCP_ACCEPT_MS));
    }
}

void ServerService::prepareForVoiceConnection()
{
    UDPsocket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (UDPsocket == INVALID_SOCKET)
    {
        pMainWindow->printOutput( std::string("ServerService::prepareForVoiceConnection::socket() error: " + std::to_string(WSAGetLastError())), true );
        return;
    }

    udpPingCheckSocket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (udpPingCheckSocket == INVALID_SOCKET)
    {
        pMainWindow->printOutput( std::string("ServerService::prepareForVoiceConnection::socket() error: " + std::to_string(WSAGetLastError())), true );
        return;
    }

    sockaddr_in myAddr;
    memset(myAddr.sin_zero, 0, sizeof(myAddr.sin_zero));
    myAddr.sin_family = AF_INET;
    myAddr.sin_port = htons(SERVER_PORT);
    myAddr.sin_addr.s_addr = htonl(INADDR_ANY);



    // Allows the socket to be bound to an address that is already in use.
    BOOL bMultAddr = true;
    if (setsockopt(UDPsocket, SOL_SOCKET, SO_REUSEADDR, reinterpret_cast<char*>(&bMultAddr), sizeof(bMultAddr)) == SOCKET_ERROR)
    {
        pMainWindow->printOutput( std::string("ServerService::prepareForVoiceConnection::setsockopt() error: " + std::to_string(WSAGetLastError())), true );
        closesocket(UDPsocket);
        return;
    }

    if (setsockopt(udpPingCheckSocket, SOL_SOCKET, SO_REUSEADDR, reinterpret_cast<char*>(&bMultAddr), sizeof(bMultAddr)) == SOCKET_ERROR)
    {
        pMainWindow->printOutput( std::string("ServerService::prepareForVoiceConnection::setsockopt() error: " + std::to_string(WSAGetLastError())), true );
        closesocket(udpPingCheckSocket);
        return;
    }




    if (bind(UDPsocket, reinterpret_cast<sockaddr*>(&myAddr), sizeof(myAddr)) == SOCKET_ERROR)
    {
        pMainWindow->printOutput( std::string("ServerService::prepareForVoiceConnection::bind() error: " + std::to_string(WSAGetLastError())), true );
        closesocket(UDPsocket);
        return;
    }

    if (bind(udpPingCheckSocket, reinterpret_cast<sockaddr*>(&myAddr), sizeof(myAddr)) == SOCKET_ERROR)
    {
        pMainWindow->printOutput( std::string("ServerService::prepareForVoiceConnection::bind() error: " + std::to_string(WSAGetLastError())), true );
        closesocket(udpPingCheckSocket);
        return;
    }



    // Translate listen socket to non-blocking mode
    u_long arg = true;
    if (ioctlsocket(UDPsocket, static_cast<long>(FIONBIO), &arg) == SOCKET_ERROR)
    {
        pMainWindow->printOutput( std::string("ServerService::prepareForVoiceConnection::ioctlsocket() error: " + std::to_string(WSAGetLastError())), true );
        closesocket(UDPsocket);
        return;
    }

    if (ioctlsocket(udpPingCheckSocket, static_cast<long>(FIONBIO), &arg) == SOCKET_ERROR)
    {
        pMainWindow->printOutput( std::string("ServerService::prepareForVoiceConnection::ioctlsocket() error: " + std::to_string(WSAGetLastError())), true );
        closesocket(udpPingCheckSocket);
        return;
    }



    bVoiceListen = true;
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
                userToListen->bConnectedToTextChat = false;
                return;
            }
            else
            {
                if (userToListen->pDataFromUser[0] == 10)
                {
                    // This is a message (in main lobby), send it to all in main lobby

                    userToListen->keepAliveTimer = clock();

                    processMessage(userToListen);
                }
            }
        }

        clock_t timePassed = clock() - userToListen->keepAliveTimer;
        float timePassedInSeconds = static_cast<float>(timePassed)/CLOCKS_PER_SEC;
        if (timePassedInSeconds > INTERVAL_KEEPALIVE_SEC)
        {
            // User was inactive for INTERVAL_KEEPALIVE_SEC seconds
            // Check if he's alive

            char keepAliveChar = 9;
            send(userToListen->userTCPSocket, &keepAliveChar, 1, 0);

            // Translate user socket to blocking mode
            u_long arg = false;
            ioctlsocket(userToListen->userTCPSocket, static_cast<long>(FIONBIO), &arg);

            // Set recv() time out time to 10 seconds
            DWORD time = MAX_TIMEOUT_TIME_MS;
            setsockopt(userToListen->userTCPSocket, SOL_SOCKET, SO_RCVTIMEO, reinterpret_cast<char*>(&time), sizeof(time));

            keepAliveChar = 0;
            int returnCode = recv(userToListen->userTCPSocket, &keepAliveChar, 1, 0);
            if (returnCode >= 0)
            {
                userToListen->keepAliveTimer = clock();

                // Translate user socket to non-blocking mode
                arg = true;
                ioctlsocket(userToListen->userTCPSocket, static_cast<long>(FIONBIO), &arg);

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

        std::this_thread::sleep_for(std::chrono::milliseconds(INTERVAL_TCP_MESSAGE_MS));
    }
}

void ServerService::listenForVoiceMessage(UserStruct *userToListen)
{
    sockaddr_in senderInfo;
    memset(senderInfo.sin_zero, 0, sizeof(senderInfo.sin_zero));
    int iLen = sizeof(senderInfo);

    char readBuffer[MAX_BUFFER_SIZE];

    // Preparation cycle
    while(userToListen->bConnectedToTextChat)
    {
        // Peeks at the incoming data. The data is copied into the buffer but is not removed from the input queue.
        // If it's data not from 'userToListen' user then we should not touch it.
        int iSize = recvfrom(UDPsocket, readBuffer, MAX_BUFFER_SIZE, MSG_PEEK, reinterpret_cast<sockaddr*>(&senderInfo), &iLen);

        if ( (iSize > 0) && (readBuffer[0] == -1)
             && (senderInfo.sin_addr.S_un.S_un_b.s_b1 == userToListen->userUDPAddr.sin_addr.S_un.S_un_b.s_b1)
             && (senderInfo.sin_addr.S_un.S_un_b.s_b2 == userToListen->userUDPAddr.sin_addr.S_un.S_un_b.s_b2)
             && (senderInfo.sin_addr.S_un.S_un_b.s_b3 == userToListen->userUDPAddr.sin_addr.S_un.S_un_b.s_b3)
             && (senderInfo.sin_addr.S_un.S_un_b.s_b4 == userToListen->userUDPAddr.sin_addr.S_un.S_un_b.s_b4) )
        {
            char userNameSize = readBuffer[1];
            char userNameBuffer[MAX_NAME_LENGTH + 1];
            memset(userNameBuffer, 0, MAX_NAME_LENGTH + 1);
            std::memcpy(userNameBuffer, readBuffer + 2, static_cast<size_t>(userNameSize));

            if ( std::string(userNameBuffer) == userToListen->userName )
            {
                iSize = recvfrom(UDPsocket, readBuffer, MAX_BUFFER_SIZE, 0, reinterpret_cast<sockaddr*>(&senderInfo), &iLen);
                userToListen->userUDPAddr.sin_port = senderInfo.sin_port;

                // Tell user
                char readyForVOIPcode = 2;
                send(userToListen->userTCPSocket, &readyForVOIPcode, 1, 0);

                // Ready to check ping
                userToListen->bConnectedToVOIP = true;

                iUsersConnectedToVOIP++;
                pMainWindow->printOutput(std::string("We are ready for VOIP with " + userToListen->userName + "."), true);

                if (iUsersConnectedToVOIP == 1)
                {
                    std::thread pingCheckThread(&ServerService::checkPing, this);
                    pingCheckThread.detach();
                }
            }

            break;
        }
    }

    while (userToListen->bConnectedToVOIP)
    {
        // Peeks at the incoming data. The data is copied into the buffer but is not removed from the input queue.
        // If it's data not from 'userToListen' user then we should not touch it.
        int iSize = recvfrom(UDPsocket, readBuffer, MAX_BUFFER_SIZE, MSG_PEEK, reinterpret_cast<sockaddr*>(&senderInfo), &iLen);

        while ( (iSize > 0)
             && (senderInfo.sin_addr.S_un.S_un_b.s_b1 == userToListen->userUDPAddr.sin_addr.S_un.S_un_b.s_b1)
             && (senderInfo.sin_addr.S_un.S_un_b.s_b2 == userToListen->userUDPAddr.sin_addr.S_un.S_un_b.s_b2)
             && (senderInfo.sin_addr.S_un.S_un_b.s_b3 == userToListen->userUDPAddr.sin_addr.S_un.S_un_b.s_b3)
             && (senderInfo.sin_addr.S_un.S_un_b.s_b4 == userToListen->userUDPAddr.sin_addr.S_un.S_un_b.s_b4)
             && (userToListen->userUDPAddr.sin_port == senderInfo.sin_port) )
        {
            if (readBuffer[0] == 0)
            {
                iSize = recvfrom(UDPsocket, readBuffer, MAX_BUFFER_SIZE, 0, reinterpret_cast<sockaddr*>(&senderInfo), &iLen);

                // it's ping check
                clock_t startPingTime;
                std::memcpy(&startPingTime, readBuffer + 1, 4);

                clock_t stopPingTime = clock() - startPingTime;
                float timePassedInMs = static_cast<float>(stopPingTime)/CLOCKS_PER_SEC;
                timePassedInMs *= 1000;

                unsigned short ping = static_cast<unsigned short>(timePassedInMs);

                userToListen->iCurrentPing = ping;
            }
            else
            {
                iSize = recvfrom(UDPsocket, readBuffer + 1 + userToListen->userName.size(), MAX_BUFFER_SIZE, 0, reinterpret_cast<sockaddr*>(&senderInfo), &iLen);

                readBuffer[0] = static_cast<char>(userToListen->userName.size());
                std::memcpy(readBuffer + 1, userToListen->userName.c_str(), userToListen->userName.size());

                iSize += 1 + userToListen->userName.size();

                int iSentSize = 0;

                for (unsigned int i = 0; i < users.size(); i++)
                {
                    if ( users[i]->userName != userToListen->userName )
                    {
                        iSentSize = sendto(UDPsocket, readBuffer, iSize, 0, reinterpret_cast<sockaddr*>(&users[i]->userUDPAddr), sizeof(users[i]->userUDPAddr));
                        if (iSentSize != iSize)
                        {
                            if (iSentSize == SOCKET_ERROR)
                            {
                                pMainWindow->printOutput(std::string("\n" + userToListen->userName + "'s voice message has not been sent to " +users[i]->userName + "!\n"
                                                                     "ServerService::listenForVoiceMessage()::sendto() failed and returned: " + std::to_string(WSAGetLastError()) + ".\n"), true);
                            }
                            else
                            {
                                pMainWindow->printOutput(std::string("\n" + userToListen->userName + "'s voice message has not been fully sent to " + users[i]->userName + "!\n"), true);
                            }
                        }
                    }
                }
            }

            iSize = recvfrom(UDPsocket, readBuffer, MAX_BUFFER_SIZE, MSG_PEEK, reinterpret_cast<sockaddr*>(&senderInfo), &iLen);
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(INTERVAL_UDP_MESSAGE_MS));
    }
}

void ServerService::processMessage(UserStruct *userToListen)
{
    // Get message size
    unsigned short int iMessageSize = 0;
    recv(userToListen->userTCPSocket, reinterpret_cast<char*>(&iMessageSize), 2, 0);


    // Get local time
    time_t now = time(nullptr);
    struct tm timeinfo;
    localtime_s(&timeinfo, &now);

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
    pSendToAllBuffer[0] = 10;
    // Set packet size to buffer
    std::memcpy(pSendToAllBuffer + 1, &iPacketSize, 2);
    // Copy time and name to buffer
    std::memcpy(pSendToAllBuffer + 3, timeString.c_str(), timeString.size());


    // Receive user message to send
    wchar_t vMessageBuffer[MAX_BUFFER_SIZE / 2];
    memset(vMessageBuffer, 0, MAX_BUFFER_SIZE / 2);


    //recv(userToListen->userTCPSocket, pSendToAllBuffer + 3 + timeString.size(), iMessageSize, 0);
    recv(userToListen->userTCPSocket, reinterpret_cast <char*> (vMessageBuffer), iMessageSize, 0);

    std::wstring sUserMessage (vMessageBuffer);

    if (pSettingsManager ->getCurrentSettings() ->bAllowHTMLInMessages == false)
    {
        for (int i = 0;   i < sUserMessage .size();   i++)
        {
            if ( sUserMessage[i] == L'<' || sUserMessage[i] == L'>' )
            {
                sUserMessage .erase( sUserMessage .begin() + i );
                i--;
            }
        }
    }

    std::memcpy(pSendToAllBuffer + 3 + timeString .size(), sUserMessage .c_str(), iMessageSize);

    int returnCode = 0;

    // Send message to all but not the sender
    for (unsigned int j = 0; j < users.size(); j++)
    {
        returnCode = send(users[j]->userTCPSocket, pSendToAllBuffer, 3 + iPacketSize, 0);
        if ( returnCode != (3 + iPacketSize) )
        {
            if (returnCode == SOCKET_ERROR)
            {
                pMainWindow->printOutput( std::string( "ServerService::getMessage::send() function failed and returned: " + std::to_string(WSAGetLastError()) ), true);
            }
            else
            {
                pMainWindow->printOutput( std::string( userToListen->userName + "'s message wasn't fully sent. send() returned: " + std::to_string(returnCode) ), true);
            }
        }
    }

    delete[] pSendToAllBuffer;
}

void ServerService::checkPing()
{
    while ( bVoiceListen && users.size() > 0 )
    {
        int iSentSize = 0;

        clock_t currentTime = clock();
        char sendBuffer[5];
        sendBuffer[0] = 0;
        std::memcpy(sendBuffer + 1, &currentTime, 4);

        for (unsigned int i = 0; i < users.size(); i++)
        {
            if ( users[i]->bConnectedToVOIP )
            {
                iSentSize = sendto(udpPingCheckSocket, sendBuffer, 5, 0, reinterpret_cast<sockaddr*>(&users[i]->userUDPAddr), sizeof(users[i]->userUDPAddr));
                if (iSentSize != 5)
                {
                    if (iSentSize == SOCKET_ERROR)
                    {
                        pMainWindow->printOutput( std::string( "ServerService::checkPing::sendto() function failed and returned: " + std::to_string(WSAGetLastError()) ), true);
                    }
                    else
                    {
                        pMainWindow->printOutput( std::string( users[i]->userName + "'s ping check wasn't fully sent. sendto() returned: " + std::to_string(iSentSize) ), true);
                    }
                }
            }

        }

        std::this_thread::sleep_for(std::chrono::seconds(PING_CHECK_INTERVAL_SEC));

        if (bVoiceListen) sendPingToAll();
    }
}

void ServerService::sendPingToAll()
{
    char sendBuffer[MAX_BUFFER_SIZE];

    // 8 means ping info
    sendBuffer[0] = 8;
    // here (in index '1-2' we will insert packet size later)
    int iCurrentPos = 3;

    for (size_t i = 0; i < users.size(); i++)
    {
        if (iCurrentPos <= (MAX_BUFFER_SIZE - 1 - static_cast<int>(users[i]->userName.size()) - static_cast<int>(sizeof(users[i]->iCurrentPing))) )
        {
            // Copy size of name
            sendBuffer[iCurrentPos] = static_cast<char>(users[i]->userName.size());
            iCurrentPos++;

            // Copy name
            std::memcpy(sendBuffer + iCurrentPos, users[i]->userName.c_str(), users[i]->userName.size());
            iCurrentPos += users[i]->userName.size();

            // Copy ping
            if (users[i]->iCurrentPing > 2000) users[i]->iCurrentPing = 0;
            std::memcpy(sendBuffer + iCurrentPos, &users[i]->iCurrentPing, sizeof(users[i]->iCurrentPing));
            iCurrentPos += sizeof(users[i]->iCurrentPing);

            pMainWindow->setPingToUser(users[i]->pListItem, users[i]->iCurrentPing);

            // Reset ping
            users[i]->iCurrentPing = 0;
        }
        else
        {
            pMainWindow->printOutput( std::string("ServerService::sendPingToAll() sendBuffer is full. " + std::to_string(i) + "/" + std::to_string(users.size()) + " users was added to buffer."), true );
            break;
        }
    }

    unsigned short iPacketSize = static_cast<unsigned short>(iCurrentPos - 3);
    // Insert packet size
    std::memcpy(sendBuffer + 1, &iPacketSize, sizeof(unsigned short));

    // Send to all
    int iSentSize = 0;

    for (size_t i = 0; i < users.size(); i++)
    {
        iSentSize = send(users[i]->userTCPSocket, sendBuffer, iCurrentPos, 0);
        if ( iSentSize != iCurrentPos )
        {
            if (iSentSize == SOCKET_ERROR)
            {
                pMainWindow->printOutput( std::string( "ServerService::sendPingToAll::send() function failed and returned: " + std::to_string(WSAGetLastError()) ), true);
            }
            else
            {
                pMainWindow->printOutput( std::string( users[i]->userName + "'s ping report wasn't fully sent. send() returned: " + std::to_string(iSentSize) ), true);
            }
        }
    }
}


void ServerService::sendFINtoSocket(SOCKET socketToClose)
{
    // Translate socket to blocking mode
    u_long arg = true;
    if (ioctlsocket(socketToClose, static_cast<long>(FIONBIO), &arg) == SOCKET_ERROR)
    {
        pMainWindow->printOutput(std::string("ServerService::sendFINtoSocket()::ioctlsocket() (Set blocking mode) function failed and returned: "
                                             + std::to_string(WSAGetLastError()) + ".\nJust closing this socket.\n"),true);
        closesocket(socketToClose);
    }
    else
    {
        int returnCode = shutdown(socketToClose, SD_SEND);
        if (returnCode == SOCKET_ERROR)
        {
            pMainWindow->printOutput(std::string("ServerService::sendFINtoSocket()::shutdown() function failed and returned: " + std::to_string(WSAGetLastError()) + "."),true);
            closesocket(socketToClose);
        }
        else
        {
            char tempBuffer[5];
            returnCode = recv(socketToClose, tempBuffer, 5, 0);
            if (returnCode == 0)
            {
                returnCode = closesocket(socketToClose);
                if (returnCode == SOCKET_ERROR)
                {
                    pMainWindow->printOutput(std::string("ServerService::sendFINtoSocket()::closesocket() function failed and returned: " + std::to_string(WSAGetLastError()) + ".\nShutdown done but can't close socket... meh. You better reboot the server...\n"),true);
                }
                else
                {
                    pMainWindow->printOutput(std::string("Received FIN and closed socket."),true);
                }
            }
            else
            {
                returnCode = recv(socketToClose, tempBuffer, 5, 0);
                if (returnCode == 0)
                {
                    returnCode = closesocket(socketToClose);
                    if (returnCode == SOCKET_ERROR)
                    {
                        pMainWindow->printOutput(std::string("ServerService::sendFINtoSocket()::closesocket() function failed and returned: " + std::to_string(WSAGetLastError()) + ".\nShutdown done but can't close socket... meh. You better reboot the server...\n"),true);

                    }
                    else
                    {
                        pMainWindow->printOutput(std::string("Received FIN and closed socket."),true);
                    }
                }
            }
        }
    }
}

void ServerService::responseToFIN(UserStruct* userToClose, bool bUserLost)
{
    if (userToClose->bConnectedToVOIP)
    {
        userToClose->bConnectedToVOIP = false;
        iUsersConnectedToVOIP--;
    }

    if (!bUserLost)
    {
        // Client sent FIN
        // We are responding:
        pMainWindow->printOutput(std::string("\n" + userToClose->userName + " has sent FIN."),true);

        int returnCode = shutdown(userToClose->userTCPSocket, SD_SEND);
        if (returnCode == SOCKET_ERROR)
        {
            pMainWindow->printOutput(std::string("ServerService::responseToFIN()::shutdown() function failed and returned: " + std::to_string(WSAGetLastError()) + "."),true);
            returnCode = shutdown(userToClose->userTCPSocket, SD_SEND);
            if (returnCode == SOCKET_ERROR)
            {
                pMainWindow->printOutput(std::string("Try #2. Can't shutdown socket. Closing socket..."),true);
                returnCode = closesocket(userToClose->userTCPSocket);
                if (returnCode == SOCKET_ERROR)
                {
                    pMainWindow->printOutput(std::string("ServerService::responseToFIN()::closesocket() function failed and returned: " + std::to_string(WSAGetLastError()) + ".\n Can't even close this socket... meh. You better reboot server...\n"),true);
                }
            }
            else
            {
                pMainWindow->printOutput(std::string("Try #2. Shutdown success."),true);
                returnCode = closesocket(userToClose->userTCPSocket);
                if (returnCode == SOCKET_ERROR)
                {
                    pMainWindow->printOutput(std::string("ServerService::responseToFIN()::closesocket() function failed and returned: " + std::to_string(WSAGetLastError()) + ".\n Can't even close this socket... meh. You better reboot server...\n"),true);
                }
                else
                {
                    pMainWindow->printOutput(std::string("Successfully closed a socket."),true);
                }
            }
        }
        else
        {
            returnCode = closesocket(userToClose->userTCPSocket);
            if (returnCode == SOCKET_ERROR)
            {
                pMainWindow->printOutput(std::string("ServerService::responseToFIN()::closesocket() function failed and returned: " + std::to_string(WSAGetLastError()) + ".\n Can't even close this socket... meh. You better reboot server...\n"),true);
            }
            else
            {
                pMainWindow->printOutput(std::string("Successfully closed connection with " + userToClose->userName + "."),true);
            }
        }
    }
    else
    {
        pMainWindow->printOutput(std::string("\nLost connection with " + userToClose->userName + ". Closing socket..."), true);
        closesocket(userToClose->userTCPSocket);
    }


    iUsersConnectedCount--;
    pMainWindow->updateOnlineUsersCount(iUsersConnectedCount);


    // Clear 'users' massive

    if (users.size() - 1 != 0)
    {
        // Tell other users that one is disconnected
        char sendBuffer[MAX_NAME_LENGTH + 3 + 5];
        memset(sendBuffer, 0, MAX_NAME_LENGTH + 3 + 5);

        // 1 means that someone is disconnected
        sendBuffer[0] = 1;

        if (bUserLost)
        {
            sendBuffer[1] = 1;
        }
        else
        {
            sendBuffer[1] = 0;
        }


        unsigned char iPacketSize = static_cast <unsigned char> (7 + userToClose->userName.size());

        std::memcpy(sendBuffer + 2, &iPacketSize, 1);

        std::memcpy(sendBuffer + 3, &iUsersConnectedCount, 4);

        std::memcpy(sendBuffer + 7, userToClose->userName.c_str(), userToClose->userName.size());

        for (size_t j = 0; j < users.size(); j++)
        {
            if ( users[j] ->userName != userToClose ->userName )
            {
               send(users[j]->userTCPSocket, sendBuffer, iPacketSize, 0);
            }
        }
    }

    // Erase user from massive
    for (unsigned int i = 0; i < users.size(); i++)
    {
        if (users[i]->userName == userToClose->userName)
        {
            delete[] userToClose->pDataFromUser;
            pMainWindow->deleteUserFromList(userToClose->pListItem);
            delete users[i];
            users.erase(users.begin() + i);
            break;
        }
    }
}

void ServerService::shutdownAllUsers()
{
    if (users.size() != 0)
    {
        pMainWindow->printOutput(std::string("\nShutting down..."));

        pMainWindow->updateOnlineUsersCount(0);

        for (unsigned int i = 0; i < users.size(); i++)
        {
            if (users[i]->bConnectedToVOIP)
            {
                users[i]->bConnectedToVOIP = false;
            }
            users[i]->bConnectedToTextChat = false;
        }

        // Now we will not listen for new sockets and we also
        // will not listen connected users (because in listenForNewMessage function while cycle will fail)
        bTextListen = false;
        bVoiceListen = false;

        std::this_thread::sleep_for(std::chrono::milliseconds(INTERVAL_TCP_ACCEPT_MS));

        closesocket(listenTCPSocket);
        closesocket(UDPsocket);
        closesocket(udpPingCheckSocket);

        iUsersConnectedToVOIP = 0;



        int correctlyClosedSocketsCount = 0;
        int socketsCount                = static_cast<int>(users.size());

        int returnCode;

        // We send FIN to all
        for (int i = 0; i < static_cast<int>(users.size()); i++)
        {
            returnCode = shutdown(users[i]->userTCPSocket,SD_SEND);
            if (returnCode == SOCKET_ERROR)
            {
                pMainWindow->printOutput(std::string("ServerService::shutdownAllUsers()::shutdown() function failed and returned: " + std::to_string(WSAGetLastError()) + ". Just closing it...."));
                closesocket(users[i]->userTCPSocket);

                // Delete user's read buffer & delete him from the list
                delete[] users[i]->pDataFromUser;

                pMainWindow->deleteUserFromList(users[i]->pListItem);
                delete users[i];

                users.erase(users.begin() + i);
                i--;
            }
        }


        // Translate all sockets to blocking mode
        for (int i = 0; i < static_cast<int>(users.size()); i++)
        {
            // Socket isn't closed
            u_long arg = false;
            if (ioctlsocket(users[i]->userTCPSocket, static_cast<long>(FIONBIO), &arg) == SOCKET_ERROR)
            {
                pMainWindow->printOutput(std::string("ServerService::shutdownAllUsers()::ioctsocket() (set blocking mode) failed and returned: " + std::to_string(WSAGetLastError()) + ". Just closing it..."));
                closesocket(users[i]->userTCPSocket);

                // Delete user's read buffer & delete him from list
                delete[] users[i]->pDataFromUser;

                pMainWindow->deleteUserFromList(users[i]->pListItem);
                delete users[i];

                users.erase(users.begin() + i);
                i--;
            }
        }

        bool tryAgainToClose = false;
        pMainWindow->printOutput(std::string("Sent FIN packets to all users. Waiting for a response..."));

        // We are waiting for a response
        for (int i = 0; i < static_cast<int>(users.size()); i++)
        {
            // Socket isn't closed
            returnCode = recv(users[i]->userTCPSocket, users[i]->pDataFromUser, 1500, 0);
            if (returnCode == 0)
            {
                // FIN received
                returnCode = closesocket(users[i]->userTCPSocket);
                if (returnCode == SOCKET_ERROR)
                {
                    pMainWindow->printOutput(std::string("ServerService::shutdownAllUsers()::closesocket() function failed and returned: " + std::to_string(WSAGetLastError()) + "."));
                }
                else
                {
                    correctlyClosedSocketsCount++;
                }

                // Delete user's read buffer & delete him from list
                delete[] users[i]->pDataFromUser;

                pMainWindow->deleteUserFromList(users[i]->pListItem);
                delete users[i];

                users.erase(users.begin() + i);
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
            for (int i = 0; i < static_cast<int>(users.size()); i++)
            {
                // Socket isn't closed
                returnCode = recv(users[i]->userTCPSocket,users[i]->pDataFromUser,1500,0);
                if (returnCode == 0)
                {
                    // FIN received
                    returnCode = closesocket(users[i]->userTCPSocket);
                    if (returnCode == SOCKET_ERROR)
                    {
                        pMainWindow->printOutput(std::string("ServerService::shutdownAllUsers()::closesocket() function failed and returned: " + std::to_string(WSAGetLastError()) + "."));
                    }
                    else
                    {
                        correctlyClosedSocketsCount++;
                    }
                }
                else
                {
                    pMainWindow->printOutput(std::string("FIN wasn't received from client for the second time... Closing socket..."));
                }

                // Delete user's read buffer & delete him from list
                delete[] users[i]->pDataFromUser;

                pMainWindow->deleteUserFromList(users[i]->pListItem);
                delete users[i];

                users.erase(users.begin() + i);
                i--;
            }
        }

        pMainWindow->printOutput(std::string("Correctly closed sockets: " + std::to_string(correctlyClosedSocketsCount) + "/" + std::to_string(socketsCount) + "."));

        // Clear users massive if someone is still there
        for (size_t i = 0; i < users.size(); i++)
        {
            delete[] users[i]->pDataFromUser;

            pMainWindow->deleteUserFromList(users[i]->pListItem);
            delete users[i];
        }

        users.clear();


        iUsersConnectedCount = 0;


        if (bWinSockStarted)
        {
            if (WSACleanup() == SOCKET_ERROR)
            {
                pMainWindow->printOutput(std::string("ServerService::shutdownAllUsers()::WSACleanup() function failed and returned: " + std::to_string(WSAGetLastError()) + "."));
            }
            else
            {
                bWinSockStarted = false;
            }
        }
    }
    else
    {
        // Now we will not listen for new sockets and we also
        // will not listen connected users (because in listenForNewMessage function while cycle will fail)
        bTextListen = false;
        bVoiceListen = false;

        closesocket(listenTCPSocket);
        closesocket(UDPsocket);
        closesocket(udpPingCheckSocket);
    }

    pMainWindow->printOutput( std::string("\nServer stopped.") );
    pMainWindow->changeStartStopActionText(false);
}
