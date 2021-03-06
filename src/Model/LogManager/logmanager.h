﻿// This file is part of the Silent Server.
// Copyright Aleksandr "Flone" Tretyakov (github.com/Flone-dnb).
// Licensed under the ZLib license.
// Refer to the LICENSE file included.

#pragma once


// STL
#include <string>
#include <mutex>



class MainWindow;
class SettingsManager;


#if _WIN32
#define TEMP_FILE_NAME L"SilentServerTempText.temp~"
#elif __linux__
#define TEMP_FILE_NAME "SilentServerTempText.temp~"
#endif


// ------------------------------------------------------------------------------------------------
// ------------------------------------------------------------------------------------------------
// ------------------------------------------------------------------------------------------------



class LogManager
{
public:

    LogManager(MainWindow* pMainWindow, SettingsManager* pSettingsManager);


    void  printAndLog   (std::string sText, bool bEmitSignal = false);
    void  archiveText   (char16_t* pText, size_t iWChars);
    void  showOldText   ();
    void  eraseTempFile ();



    void  stop          ();

private:

    void  logThread     (std::string sText);
    void  archiveThread (char16_t* pText, size_t iWChars);
    void  showTextThread();



    MainWindow*      pMainWindow;
    SettingsManager* pSettingsManager;


    std::mutex       mtxLogWrite;
};
