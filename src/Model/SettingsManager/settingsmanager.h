// This file is part of the Silent Server.
// Copyright Aleksandr "Flone" Tretyakov (github.com/Flone-dnb).
// Licensed under the ZLib license.
// Refer to the LICENSE file included.

#pragma once


// STL
#include <mutex>


class MainWindow;
class SettingsFile;


#if _WIN32
#define SETTINGS_NAME u"SilentServerSettings.data"
#elif __linux__
#define SETTINGS_NAME "SilentServerSettings.data"
#endif


// ------------------------------------------------------------------------------------------------
// ------------------------------------------------------------------------------------------------
// ------------------------------------------------------------------------------------------------


class SettingsManager
{

public:

    SettingsManager(MainWindow* pMainWindow);
    ~SettingsManager();


    // SET functions

        void          saveSettings        (SettingsFile* pSettingsFile);


    // GET functions

        SettingsFile* getCurrentSettings  ();


private:


    SettingsFile*  readSettings  ();


    // ----------------------------------


    MainWindow*    pMainWindow;
    SettingsFile*  pCurrentSettingsFile;


    std::mutex     mtxUpdateSettings;
};
