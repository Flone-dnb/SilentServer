#include "serverservice.h"

//Custom
#include "View/MainWindow/mainwindow.h"

//C++
#include <string>
#include <thread>


ServerService::ServerService(MainWindow* pMainWindow)
{
    this->pMainWindow = pMainWindow;

    bWinSockStarted = false;
    bListening      = false;

    iUsersConnectedCount = 0;
}

void ServerService::startWinSock()
{
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
        if (bListening)
        {
            std::thread listenThread(&ServerService::listenForNewConnections,this);
            listenThread.detach();
        }
    }
}

void ServerService::startToListenForConnection()
{
    // Create the IPv4 TCP socket
    listenSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (listenSocket == INVALID_SOCKET)
    {
        pMainWindow->printOutput(std::string("ServerService::listenForConnection()::socket() function failed and returned: " + std::to_string(WSAGetLastError()) + "."));
    }
    else
    {
        pMainWindow->printOutput(std::string("Created listen socket..."));

        // Create and fill the "sockaddr_in" structure containing the IPv4 socket
        sockaddr_in myAddr;
        memset(myAddr.sin_zero, 0, sizeof(myAddr.sin_zero));
        myAddr.sin_family = AF_INET;
        // if port == 0 then that means that it will find free port
        myAddr.sin_port = htons(51337);
        myAddr.sin_addr.s_addr = htonl(INADDR_ANY);

        if (bind(listenSocket, reinterpret_cast<sockaddr*>(&myAddr), sizeof(myAddr)) == SOCKET_ERROR)
        {
            pMainWindow->printOutput(std::string("ServerService::listenForConnection()::bind() function failed and returned: " + std::to_string(WSAGetLastError()) + ".\nSocket will be closed. Try again.\n"));
            closesocket(listenSocket);
        }
        else
        {
            // Find out local port and show it
            sockaddr_in myBindedAddr;
            int len = sizeof(myBindedAddr);
            getsockname(listenSocket, reinterpret_cast<sockaddr*>(&myBindedAddr), &len);

            // Get my IP
            char myIP[16];
            inet_ntop(AF_INET,&myBindedAddr.sin_addr,myIP,sizeof(myIP));

            pMainWindow->printOutput(std::string("Success. Waiting for a connection requests on port: " + std::to_string(ntohs(myBindedAddr.sin_port)) + "."));

            if (listen(listenSocket, SOMAXCONN) == SOCKET_ERROR)
            {
                pMainWindow->printOutput(std::string("ServerService::listenForConnection()::listen() function failed and returned: " + std::to_string(WSAGetLastError()) + ".\nSocket will be closed. Try again.\n"));
                closesocket(listenSocket);
            }
            else
            {
                pMainWindow->printOutput(std::string("\n!!!!!!!!WARNING!!!!!!!!\nThe data transmitted over the network is not encrypted.\n"));

                // Translate listen socket to non-blocking mode
                u_long arg = true;
                if (ioctlsocket(listenSocket,FIONBIO,&arg) == SOCKET_ERROR)
                {
                    pMainWindow->printOutput(std::string("ServerService::listenForConnection()::ioctsocket() failed and returned: " + std::to_string(WSAGetLastError()) + ".\nSocket will be closed. Try again.\n"));
                    closesocket(listenSocket);
                }
                else
                {
                    bListening = true;
                }
            }
        }
    }
}

void ServerService::listenForNewConnections()
{
    sockaddr_in connectedWith;
    memset(connectedWith.sin_zero, 0, sizeof(connectedWith.sin_zero));
    int iLen = sizeof(connectedWith);

    // Accept new connection
    while (bListening)
    {
        // We will check if there is a message every 500 ms (look end of this function)
        SOCKET newConnectedSocket;
        newConnectedSocket = accept(listenSocket, reinterpret_cast<sockaddr*>(&connectedWith), &iLen);
        if (newConnectedSocket != INVALID_SOCKET)
        {
            pMainWindow->printOutput(std::string("\nSomeone is connecting..."),true);
            // Disable Nagle algorithm for Connected Socket
            BOOL bOptVal = true;
            int bOptLen = sizeof(BOOL);
            if (setsockopt(newConnectedSocket, IPPROTO_TCP, TCP_NODELAY, reinterpret_cast<char*>(&bOptVal), bOptLen) == SOCKET_ERROR)
            {
                pMainWindow->printOutput(std::string("ServerService::listenForNewConnections()::setsockopt() (Nagle algorithm) failed and returned: " + std::to_string(WSAGetLastError()) + ".\nSending FIN to this new user.\n"),true);

                std::thread closethread(&ServerService::sendFINtoSocket,this,std::ref(newConnectedSocket));
                closethread.detach();
            }
            else
            {
                // Receive user name
                char* pNameBuffer = new char[21];
                memset(pNameBuffer,0,21);
                if (recv(newConnectedSocket,pNameBuffer,20,0) > 1)
                {
                    // Received user name

                    // Check if this user name is free
                    std::string userNameStr(pNameBuffer);
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
                        pMainWindow->printOutput(std::string("New user name collision."),true);
                        char command = 0;
                        send(newConnectedSocket,reinterpret_cast<char*>(&command),1,0);
                        std::thread closethread(&ServerService::sendFINtoSocket,this,std::ref(newConnectedSocket));
                        closethread.detach();
                    }
                    else
                    {
                        // Show with whom connected
                        char connectedWithIP[16];
                        memset(&connectedWithIP,0,16);
                        inet_ntop(AF_INET, &connectedWith.sin_addr, connectedWithIP, sizeof(connectedWithIP));

                        char* pTempData = new char[1500];
                        memset(pTempData,0,1500);

                        // we ++ new user (if something will go wrong later we will -- this user
                        iUsersConnectedCount++;


                        // Prepare online info to user.
                        // Prepared data format (amount of bytes in '()'):

                        // (1) Is user name free (if not then all other stuff is not included)
                        // (2) Packet size minus "free name" byte (and excluding packet size)
                        // (4) Amount of users in main lobby (online)
                        // [
                        //      (1) Size in bytes of user name online
                        //      (usernamesize) user name
                        // ]

                        int iBytesWillSend = 0;
                        char command = 1;
                        std::memcpy(pTempData, &command, 1);
                        iBytesWillSend++;

                        // We will put here packet size
                        iBytesWillSend += 2;

                        std::memcpy(pTempData + iBytesWillSend, &iUsersConnectedCount, 4);
                        iBytesWillSend += 4;
                        for (unsigned int j = 0; j < users.size(); j++)
                        {
                            unsigned char nameSize = static_cast<unsigned char>(users[j]->userName.size()) + 1;
                            std::memcpy(pTempData+iBytesWillSend,&nameSize,1);
                            iBytesWillSend++;

                            std::memcpy(pTempData+iBytesWillSend,users[j]->userName.c_str(),nameSize);
                            iBytesWillSend+=nameSize;
                        }

                        // Put packet size to buffer (packet size - command size (1 byte) - packet size (2 bytes))
                        unsigned short int iPacketSize = static_cast<unsigned short>(iBytesWillSend - 3);
                        std::memcpy(pTempData + 1, &iPacketSize, 2);

                        // SEND
                        int iBytesWereSent = send(newConnectedSocket,pTempData,iBytesWillSend,0);
                        if (iBytesWillSend > 1200)
                        {
                            pMainWindow->printOutput(std::string("\nWARNING:\n"+std::to_string(iBytesWillSend)+" bytes were sent to new user. Think about increasing read buffers or compressing data before it's sent, because current buffers size is 1500 bytes.\n"),true);
                        }
                        if (iBytesWillSend != iBytesWereSent)
                        {
                            pMainWindow->printOutput(std::string("\nWARNING:\n"+std::to_string(iBytesWereSent)+" bytes were sent of total "+std::to_string(iBytesWillSend)+" to new user.\n"),true);
                        }
                        if (iBytesWillSend == -1)
                        {
                            pMainWindow->printOutput(std::string("ServerService::listenForNewConnections()::send()) (online info) failed and returned: "+std::to_string(WSAGetLastError())+"."),true);
                            if (recv(newConnectedSocket,pTempData,1500,0) == 0)
                            {
                                pMainWindow->printOutput(std::string("received FIN from this new user who didn't receive online info."),true);
                                shutdown(newConnectedSocket,SD_SEND);
                                if (closesocket(newConnectedSocket) != SOCKET_ERROR)
                                {
                                    pMainWindow->printOutput(std::string("closed this socket with success."),true);
                                }
                                else
                                {
                                    pMainWindow->printOutput(std::string("can't close this socket... meh. You better reboot the server..."),true);
                                }
                            }
                            iUsersConnectedCount--;
                        }
                        else
                        {
                            // Translate new connected socket to non-blocking mode
                            u_long arg = true;
                            if (ioctlsocket(newConnectedSocket,FIONBIO,&arg) == SOCKET_ERROR)
                            {
                                pMainWindow->printOutput(std::string("ServerService::listenForNewConnections()::ioctsocket() (non-blocking mode) failed and returned: " + std::to_string(WSAGetLastError()) + "."),true);

                                std::thread closethread(&ServerService::sendFINtoSocket,this,std::ref(newConnectedSocket));
                                closethread.detach();
                            }
                            else
                            {
                                if (users.size() != 0)
                                {
                                    // Tell other users about new user

                                    char* pNewUserInfo = new char[30];
                                    memset(pNewUserInfo, 0, 30);

                                    unsigned char sizeOfUserName = static_cast<unsigned char>(std::string(pNameBuffer).size());

                                    unsigned char iSendSize = 0;

                                    // 0 - means 'there is new user, update your OnlineCount and add him to list
                                    unsigned char commandType = 0;
                                    std::memcpy(pNewUserInfo, &commandType, 1);
                                    iSendSize++;

                                    // Put packet size
                                    unsigned char iPacketSize = 4 + 1 + sizeOfUserName;
                                    std::memcpy(pNewUserInfo + 1, &iPacketSize, 1);
                                    iSendSize++;

                                    std::memcpy(pNewUserInfo + 2, &iUsersConnectedCount, 4);
                                    iSendSize += 4;

                                    std::memcpy(pNewUserInfo + 6, &sizeOfUserName, 1);
                                    iSendSize++;
                                    std::memcpy(pNewUserInfo + 7, pNameBuffer, sizeOfUserName);
                                    iSendSize += sizeOfUserName;

                                    // Send this data
                                    for (unsigned int i = 0; i < users.size(); i++)
                                    {
                                        send(users[i]->userSocket, pNewUserInfo, iSendSize, 0);
                                    }

                                    delete[] pNewUserInfo;
                                }


                                // Fill UserStruct for new user

                                users.push_back(new UserStruct());
                                users[ users.size() - 1 ]->userName      = std::string(pNameBuffer);
                                users[ users.size() - 1 ]->userSocket    = newConnectedSocket;
                                users[ users.size() - 1 ]->pDataFromUser = new char[1500];
                                users[ users.size() - 1 ]->userIP        = std::string(connectedWithIP);
                                users[ users.size() - 1 ]->userPort      = ntohs(connectedWith.sin_port);

                                // Ready to send and receive data

                                pMainWindow->printOutput(std::string("Connected with " + std::string(connectedWithIP) + ":" + std::to_string(ntohs(connectedWith.sin_port)) + " AKA " + users[ users.size() - 1 ]->userName + "."),true);
                                pMainWindow->updateOnlineUsersCount(iUsersConnectedCount);
                                users[ users.size() - 1 ]->pListItem = pMainWindow->addNewUserToList(users[ users.size() - 1 ]->userName);

                                std::thread listenThread(&ServerService::listenForMessage, this, users[ users.size() - 1], std::ref(mtxUsersWrite));
                                listenThread.detach();
                            }
                        }
                        delete[] pTempData;
                    }
                }
                delete[] pNameBuffer;
            }
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(500));
    }
}

void ServerService::listenForMessage(UserStruct* userToListen, std::mutex& mtxUsersWrite)
{
    while(bListening)
    {
        // We will check if there is a message every 10 ms (look end of this function)
        if (recv(userToListen->userSocket, userToListen->pDataFromUser,0,0) == 0)
        {
            // There are some data to read

            // We want to receive only 1 byte of data that contains packet ID (what packet it is: message or something else).
            // After that, if it's a message then we will receive another 2 bytes (size of the message) and receive another "size of the message" bytes.
            // We do that because TCP is a stream. All data that users sent to us is written in one stream and we read from it, we don't read separate packets, like in UDP.
            // Example: if we would set "len" param in recv() to 400 bytes and user simultaneously will send to us 2 messages with size < 200 those 2 messages will be written in our TCP stream
            // and because we set "len" to 400 we will read 2 messages in one buffer in one recv() call. We don't want that.
            // Because of that first, we read packet ID, second - packet size, third - message.
            int receivedAmount = recv(userToListen->userSocket, userToListen->pDataFromUser,1,0);
            if (receivedAmount == 0)
            {
                // Client sent FIN
                mtxUsersWrite.lock();

                responseToFIN(userToListen);

                if (users.size() - 1 != 0)
                {
                    // Tell other users that one is disconnected
                    char* pSendInfo = new char[30];
                    memset(pSendInfo, 0, 30);

                    // 1 means that someone is disconnected
                    unsigned char commandType = 1;
                    std::memcpy(pSendInfo, &commandType, 1);

                    // Put size
                    unsigned char iPacketSize = static_cast<unsigned char>(4 + userToListen->userName.size());
                    std::memcpy(pSendInfo + 1, &iPacketSize, 1);

                    std::memcpy(pSendInfo + 2, &iUsersConnectedCount, 4);

                    std::memcpy(pSendInfo + 6, userToListen->userName.c_str(), userToListen->userName.size());

                    for (unsigned int j = 0; j < users.size(); j++)
                    {
                        send(users[j]->userSocket, pSendInfo, 1 + 1 + 4 + userToListen->userName.size(), 0);
                    }

                    delete[] pSendInfo;
                }

                // Erase user from massive
                for (unsigned int i = 0; i < users.size(); i++)
                {
                    if (users[i]->userName == userToListen->userName)
                    {
                        delete[] userToListen->pDataFromUser;
                        pMainWindow->deleteUserFromList(userToListen->pListItem);
                        delete users[i];
                        users.erase(users.begin() + i);
                        break;
                    }
                }

                mtxUsersWrite.unlock();
                // Stop thread
                return;
            }
            else
            {
                if (userToListen->pDataFromUser[0] == 10)
                {
                    // This is a message (in main lobby), send it to all in main lobby
                    // Get message size
                    receivedAmount = recv(userToListen->userSocket, userToListen->pDataFromUser, 2, 0);
                    unsigned short int iPacketSize = 0;
                    std::memcpy(&iPacketSize, userToListen->pDataFromUser, 2);

                    // Resend it
                    char* pResendToAll = new char[1 + 2 + iPacketSize];
                    receivedAmount = recv(userToListen->userSocket, userToListen->pDataFromUser, iPacketSize, 0);

                    pResendToAll[0] = 10;
                    std::memcpy(pResendToAll + 1, &iPacketSize, 2);
                    std::memcpy(pResendToAll + 3, userToListen->pDataFromUser, iPacketSize);

                    for (unsigned int j = 0; j < users.size(); j++)
                    {
                        if (users[j]->userName != userToListen->userName)
                        {
                          send(users[j]->userSocket, pResendToAll, 1 + 2 + iPacketSize, 0);
                        }
                    }

                    delete[] pResendToAll;
                }
            }
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
}



void ServerService::sendFINtoSocket(SOCKET& socketToClose)
{
    int returnCode = shutdown(socketToClose,SD_SEND);
    if (returnCode == SOCKET_ERROR)
    {
        pMainWindow->printOutput(std::string("ServerService::sendFINtoSocket()::shutdown() function failed and returned: " + std::to_string(WSAGetLastError()) + "."),true);
        closesocket(socketToClose);
    }
    else
    {
        // Translate socket to blocking mode
        u_long arg = true;
        if (ioctlsocket(socketToClose,FIONBIO,&arg) == SOCKET_ERROR)
        {
            pMainWindow->printOutput(std::string("ServerService::sendFINtoSocket()::ioctlsocket() (Set blocking mode) function failed and returned: "
                                                 + std::to_string(WSAGetLastError()) + ".\nJust closing this socket.\n"),true);
            closesocket(socketToClose);
        }
        else
        {
            char* pTempBuffer = new char[1500];
            returnCode = recv(socketToClose,pTempBuffer,1500,0);
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
                returnCode = recv(socketToClose,pTempBuffer,1500,0);
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
            delete[] pTempBuffer;
        }
    }
}

void ServerService::responseToFIN(UserStruct* userToClose)
{
    // Client sent FIN
    // We are responding:
    pMainWindow->printOutput(std::string(userToClose->userName + " has sent FIN."),true);

    int returnCode = shutdown(userToClose->userSocket,SD_SEND);
    if (returnCode == SOCKET_ERROR)
    {
        pMainWindow->printOutput(std::string("ServerService::responseToFIN()::shutdown() function failed and returned: " + std::to_string(WSAGetLastError()) + "."),true);
        returnCode = shutdown(userToClose->userSocket,SD_SEND);
        if (returnCode == SOCKET_ERROR)
        {
            pMainWindow->printOutput(std::string("Try #2. Can't shutdown socket. Closing socket..."),true);
            returnCode = closesocket(userToClose->userSocket);
            if (returnCode == SOCKET_ERROR)
            {
                pMainWindow->printOutput(std::string("ServerService::responseToFIN()::closesocket() function failed and returned: " + std::to_string(WSAGetLastError()) + ".\n Can't even close this socket... meh. You better reboot server...\n"),true);
            }
        }
        else
        {
            pMainWindow->printOutput(std::string("Try #2. Shutdown success."),true);
            returnCode = closesocket(userToClose->userSocket);
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
        returnCode = closesocket(userToClose->userSocket);
        if (returnCode == SOCKET_ERROR)
        {
            pMainWindow->printOutput(std::string("ServerService::responseToFIN()::closesocket() function failed and returned: " + std::to_string(WSAGetLastError()) + ".\n Can't even close this socket... meh. You better reboot server...\n"),true);
        }
        else
        {
            pMainWindow->printOutput(std::string("Successfully closed connection with " + userToClose->userName + "."),true);
        }
    }

    iUsersConnectedCount--;
    pMainWindow->updateOnlineUsersCount(iUsersConnectedCount);
}

void ServerService::shutdownAllUsers()
{
    if (users.size() != 0)
    {
        pMainWindow->printOutput(std::string("Stating to close all sockets.\nPlease don't close programm until all sockets will be closed.\n"
                                             "The delay may be caused by the fact that some user does not send the FIN packet."));
        // Now we will not listen for new sockets and we also
        // will not listen connected users (because in listenForNewMessage function while cycle will fail)
        bListening = false;

        std::vector<bool> closedSockets(users.size());
        int correctlyClosedSocketsCount = 0;

        int returnCode;

        // We send FIN to all
        for (unsigned int i = 0; i < users.size(); i++)
        {
            returnCode = shutdown(users[i]->userSocket,SD_SEND);
            if (returnCode == SOCKET_ERROR)
            {
                pMainWindow->printOutput(std::string("ServerService::shutdownAllUsers()::shutdown() function failed and returned: " + std::to_string(WSAGetLastError()) + ". Just closing it...."));
                closesocket(users[i]->userSocket);

                // There was an error. We emergency shut it down.
                closedSockets[i] = true;

                // Delete user's read buffer & delete him from list
                delete[] users[i]->pDataFromUser;

                pMainWindow->deleteUserFromList(users[i]->pListItem);
                delete users[i];
            }
        }


        // Translate all sockets to blocking mode
        for (unsigned int i = 0; i < users.size(); i++)
        {
            if (closedSockets[i] == false)
            {
                // Socket isn't closed
                u_long arg = false;
                if (ioctlsocket(users[i]->userSocket,FIONBIO,&arg) == SOCKET_ERROR)
                {
                    pMainWindow->printOutput(std::string("ServerService::shutdownAllUsers()::ioctsocket() (set blocking mode) failed and returned: " + std::to_string(WSAGetLastError()) + ". Just closing it..."));
                    closesocket(users[i]->userSocket);

                    // There was an error. We emergency shut it down.
                    closedSockets[i] = true;

                    // Delete user's read buffer & delete him from list
                    delete[] users[i]->pDataFromUser;

                    pMainWindow->deleteUserFromList(users[i]->pListItem);
                    delete users[i];
                }
            }
        }

        bool tryAgainToClose = false;
        pMainWindow->printOutput(std::string("Sent FIN packets to all users. Waiting for a response..."));

        // We are waiting for a response
        for (unsigned int i = 0; i < users.size(); i++)
        {
            if (closedSockets[i] == false)
            {
                // Socket isn't closed
                returnCode = recv(users[i]->userSocket,users[i]->pDataFromUser,1500,0);
                if (returnCode == 0)
                {
                    // FIN received
                    returnCode = closesocket(users[i]->userSocket);
                    if (returnCode == SOCKET_ERROR)
                    {
                        pMainWindow->printOutput(std::string("ServerService::shutdownAllUsers()::closesocket() function failed and returned: " + std::to_string(WSAGetLastError()) + "."));
                    }
                    else
                    {
                        correctlyClosedSocketsCount++;
                    }

                    // There was an error. We emergency shut it down.
                    closedSockets[i] = true;

                    // Delete user's read buffer & delete him from list
                    delete[] users[i]->pDataFromUser;

                    pMainWindow->deleteUserFromList(users[i]->pListItem);
                    delete users[i];
                }
                else
                {
                    tryAgainToClose = true;
                }
            }
        }

        if (tryAgainToClose)
        {
            // Try again to close the sockets that does not returned FIN
            for (unsigned int i = 0; i < users.size(); i++)
            {
                if (closedSockets[i] == false)
                {
                    // Socket isn't closed
                    returnCode = recv(users[i]->userSocket,users[i]->pDataFromUser,1500,0);
                    if (returnCode == 0)
                    {
                        // FIN received
                        returnCode = closesocket(users[i]->userSocket);
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

                    // There was an error. We emergency shut it down.
                    closedSockets[i] = true;

                    // Delete user's read buffer & delete him from list
                    delete[] users[i]->pDataFromUser;

                    pMainWindow->deleteUserFromList(users[i]->pListItem);
                    delete users[i];
                }
            }
        }

        pMainWindow->printOutput(std::string("Correctly closed sockets: " + std::to_string(correctlyClosedSocketsCount) + "/" + std::to_string(users.size()) + "."));

        closesocket(listenSocket);

        // Clear users massive
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
}
