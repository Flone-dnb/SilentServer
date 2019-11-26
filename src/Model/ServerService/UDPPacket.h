#pragma once


// STL
#include <vector>
#include <string>

// Custom
#include "../src/Model/net_params.h"
#include <Ws2def.h> // sockaddr_in



class ThreadReject
{
public:

    ThreadReject(std::string sUserName)
    {
        this ->sUserName = sUserName;
        bRejected        = true;
    }


    std::string sUserName;
    bool        bRejected;
};


// So, we use this class and not raw recvfrom() to get UDP packets because the Server is designed that way
// that every thread is controlling one user and if a thread gets the wrong packet (packet from the user that
// this thread is not controlling) it will not touch this packet and just wait until it's gone from packet queue.
// This is a problem because our UDP socket is "connectionless" and it's not bound to any remote point (unlike our TCP sockets).
// And so when we receive a UDP packet not from our users (say it's some bad guy or just a weird coincidence) then
// every thread will reject this packet and will infinitely wait until this packet will be gone from the packet queue and
// so all VOIP will not work.


class UDPPacket
{
public:

    UDPPacket()
    {
        memset(senderInfo.sin_zero, 0, sizeof(senderInfo.sin_zero));
        iLen = sizeof(senderInfo);
    }

    bool checkRejected(const std::string& sUserName)
    {
        for (size_t i = 0; i < vThreadsRejectedPacket .size(); i++)
        {
            if (vThreadsRejectedPacket[i] .sUserName == sUserName)
            {
                return true;
            }
        }

        return false;
    }

    void rejectPacket(const std::string& sUserName)
    {
        vThreadsRejectedPacket .push_back( ThreadReject(sUserName) );
    }


    // ------------------------------


    std::vector<ThreadReject> vThreadsRejectedPacket;


    // Packet info.
    char        vPacketData[MAX_BUFFER_SIZE];
    int         iSize;


    // Sender info.
    sockaddr_in senderInfo;
    int         iLen;
};
