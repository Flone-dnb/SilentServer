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
