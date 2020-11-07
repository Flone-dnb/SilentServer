// This file is part of the Silent Server.
// Copyright Aleksandr "Flone" Tretyakov (github.com/Flone-dnb).
// Licensed under the ZLib license.
// Refer to the LICENSE file included.

#pragma once


// STL
#include <string>

// Custom
#include "../src/Model/net_params.h"

#define SETTINGS_FILE_MAGIC_NUMBER 999999
#define SETTINGS_FILE_VERSION 1

class SettingsFile
{
public:

#if _WIN32
    SettingsFile(unsigned short int iPort     = SERVER_PORT,
                 bool bAllowHTMLInMessages    = false,
                 std::u16string sPasswordToJoin = u"",
                 bool bLog                    = false,
                 std::u16string sPathToLogFile  = u"")
    {
        this ->iPort                = iPort;
        this ->bAllowHTMLInMessages = bAllowHTMLInMessages;
        this ->sPasswordToJoin      = sPasswordToJoin;
        this ->bLog                 = bLog;
        this ->sPathToLogFile       = sPathToLogFile;
    }
#elif __linux__
    SettingsFile(unsigned short int iPort     = SERVER_PORT,
                 bool bAllowHTMLInMessages    = false,
                 std::u16string sPasswordToJoin = u"",
                 bool bLog                    = false,
                 std::string sPathToLogFile   = "")
    {
        this ->iPort                = iPort;
        this ->bAllowHTMLInMessages = bAllowHTMLInMessages;
        this ->sPasswordToJoin      = sPasswordToJoin;
        this ->bLog                 = bLog;
        this ->sPathToLogFile       = sPathToLogFile;
    }
#endif


    // -------------------------


    std::u16string     sPasswordToJoin = u"";
#if _WIN32
    std::u16string     sPathToLogFile = u"";
#elif __linux__
    std::string        sPathToLogFile;
#endif

    unsigned short int iPort;


    bool               bAllowHTMLInMessages;
    bool               bLog;
};
