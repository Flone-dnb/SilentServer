#pragma once


// Custom
#include "../src/Model/net_params.h"


class SettingsFile
{
public:

    SettingsFile(unsigned short int iPort  = SERVER_PORT,
                 bool bAllowHTMLInMessages = true)
    {
        this ->iPort                = iPort;
        this ->bAllowHTMLInMessages = bAllowHTMLInMessages;
    }


    // -------------------------

    unsigned short int iPort;
    bool               bAllowHTMLInMessages;
};
