// This file is part of the Silent Server.
// Copyright Aleksandr "Flone" Tretyakov (github.com/Flone-dnb).
// Licensed under the ZLib license.
// Refer to the LICENSE file included.

#pragma once


// STL
#include <string>

// Custom
#include "../src/Model/net_params.h"

#if _WIN32
#define S16String std::wstring
#define S16Char   wchar_t
#elif __linux__
#define S16String std::u16string
#define S16Char   char16_t
#endif


class SettingsFile
{
public:

#if _WIN32
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


    S16String          sPasswordToJoin;
#if _WIN3
    std::wstring       sPathToLogFile;
#elif __linux__
    std::string        sPathToLogFile;
#endif

    unsigned short int iPort;


    bool               bAllowHTMLInMessages;
    bool               bLog;
};
