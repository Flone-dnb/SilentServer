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
                 std::wstring sPasswordToJoin = L"",
                 bool bLog                    = false,
                 std::wstring sPathToLogFile  = L"")
    {
        this ->iPort                = iPort;
        this ->bAllowHTMLInMessages = bAllowHTMLInMessages;
        this ->sPasswordToJoin      = sPasswordToJoin;
        this ->bLog                 = bLog;
        this ->sPathToLogFile       = sPathToLogFile;
    }


    // -------------------------


    std::wstring       sPasswordToJoin;
    std::wstring       sPathToLogFile;


    unsigned short int iPort;


    bool               bAllowHTMLInMessages;
    bool               bLog;
};
