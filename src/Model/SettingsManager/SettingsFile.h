#pragma once


// STL
#include <string>

// Custom
#include "../src/Model/net_params.h"


class SettingsFile
{
public:

    SettingsFile(unsigned short int iPort     = SERVER_PORT,
                 bool bAllowHTMLInMessages    = false,
                 std::wstring sPasswordToJoin = L"")
    {
        this ->iPort                = iPort;
        this ->bAllowHTMLInMessages = bAllowHTMLInMessages;
        this ->sPasswordToJoin      = sPasswordToJoin;
    }


    // -------------------------


    std::wstring       sPasswordToJoin;


    unsigned short int iPort;


    bool               bAllowHTMLInMessages;
};
