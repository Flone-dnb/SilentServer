#pragma once


// Custom
#include "../src/Model/net_params.h"


class SettingsFile
{
public:

    SettingsFile(unsigned short int iPort = SERVER_PORT)
    {
        this ->iPort = iPort;
    }


    // -------------------------

    unsigned short int iPort;
};
