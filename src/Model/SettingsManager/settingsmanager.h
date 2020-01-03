// This file is part of the Silent Server.
// Copyright Aleksandr "Flone" Tretyakov (github.com/Flone-dnb).
// Licensed under the ZLib license.
// Refer to the LICENSE file included.

#pragma once


// STL
#include <mutex>


class MainWindow;
class SettingsFile;



#define SETTINGS_NAME L"SilentServerSettings.data"


// ------------------------------------------------------------------------------------------------
// ------------------------------------------------------------------------------------------------
// ------------------------------------------------------------------------------------------------


class SettingsManager
{

public:

    SettingsManager(MainWindow* pMainWindow);


    // SET functions

        void          saveSettings (SettingsFile* pSettingsFile);


    // GET functions

        SettingsFile* getCurrentSettings  ();


private:


    SettingsFile*  readSettings  ();


    // ----------------------------------


    MainWindow*    pMainWindow;
    SettingsFile*  pCurrentSettingsFile;


    std::mutex     mtxUpdateSettings;
};
