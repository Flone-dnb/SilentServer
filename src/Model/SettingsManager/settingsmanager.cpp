// This file is part of the Silent Server.
// Copyright Aleksandr "Flone" Tretyakov (github.com/Flone-dnb).
// Licensed under the ZLib license.
// Refer to the LICENSE file included.

#include "settingsmanager.h"


// STL
#include <fstream>

#if _WIN32
#include <shlobj.h>
#elif __linux__
#define MAX_PATH 255
#include <stdlib.h> // for getenv()
#include <stdio.h>
#include <unistd.h>
#endif

// Custom
#include "View/MainWindow/mainwindow.h"
#include "Model/SettingsManager/SettingsFile.h"

// ------------------------------------------------------------------------------------------------
// ------------------------------------------------------------------------------------------------
// ------------------------------------------------------------------------------------------------


SettingsManager::SettingsManager(MainWindow* pMainWindow)
{
    this ->pMainWindow = pMainWindow;


    pCurrentSettingsFile = nullptr;
    pCurrentSettingsFile = readSettings();
}







void SettingsManager::saveSettings(SettingsFile *pSettingsFile)
{
#if _WIN32
    // Get the path to the Documents folder.

    TCHAR   my_documents[MAX_PATH];
    HRESULT result = SHGetFolderPathW( nullptr, CSIDL_PERSONAL, nullptr, SHGFP_TYPE_CURRENT, my_documents );

    if (result != S_OK)
    {
        pMainWindow ->showMessageBox(true, L"ServerService::showSettings::SHGetFolderPathW() failed. "
                                           "Can't open the settings file.");

        delete pSettingsFile;

        return;
    }

    // Open the settings file.

    std::wstring sPathToOldSettings  = std::wstring(my_documents);
    std::wstring sPathToNewSettings  = std::wstring(my_documents);

    sPathToOldSettings  += L"\\" + std::wstring(SETTINGS_NAME);
    sPathToNewSettings  += L"\\" + std::wstring(SETTINGS_NAME) + L"~";
#elif __linux__
    char* pHomeDir = NULL;
    pHomeDir = getenv("HOME");

    std::string sPathToOldSettings(pHomeDir);
    std::string sPathToNewSettings(pHomeDir);

    if (pHomeDir)
    {
        sPathToOldSettings += "/" + std::string(SETTINGS_NAME);
        sPathToNewSettings += "/" + std::string(SETTINGS_NAME) + "~";
    }
    else
    {
#if _WIN32
        pMainWindow ->showMessageBox(true, L"ServerService::showSettings() failed. "
                                           "Can't get path to HOME.");
#elif __linux__
        pMainWindow ->showMessageBox(true, u"ServerService::showSettings() failed. "
                                           "Can't get path to HOME.");
#endif

        delete pSettingsFile;

        return;
    }
#endif

    std::ifstream settingsFile (sPathToOldSettings, std::ios::binary);
    std::ofstream newSettingsFile;

    if ( settingsFile .is_open() )
    {
        newSettingsFile .open(sPathToNewSettings);
    }
    else
    {
        newSettingsFile .open(sPathToOldSettings);
    }




    // Write new setting to the new file.

    // Server port
    newSettingsFile .write
            ( reinterpret_cast <char*> (&pSettingsFile ->iPort), sizeof(pSettingsFile ->iPort) );

    // Allow HTML in messages
    char cAllowHTMLInMessages = pSettingsFile ->bAllowHTMLInMessages;
    newSettingsFile .write
            ( &cAllowHTMLInMessages, sizeof(cAllowHTMLInMessages) );

    // Password to join
    unsigned char cPasswordToJoinLength = static_cast <unsigned char> ( pSettingsFile ->sPasswordToJoin .size() );
    newSettingsFile .write
            ( reinterpret_cast<char*>(&cPasswordToJoinLength), sizeof(cPasswordToJoinLength) );

#if _WIN32
    newSettingsFile .write
            ( reinterpret_cast<char*>(  const_cast <wchar_t*>(pSettingsFile ->sPasswordToJoin .c_str())  ), cPasswordToJoinLength * 2 );
#elif __linux__
    newSettingsFile .write
            ( reinterpret_cast<char*>(  const_cast <char16_t*>(pSettingsFile ->sPasswordToJoin .c_str())  ), cPasswordToJoinLength * 2 );
#endif

    // Log
    char cLog = pSettingsFile ->bLog;
    newSettingsFile .write
            ( &cLog, sizeof(cLog) );

    if (pSettingsFile ->bLog)
    {
       unsigned char cLogFilePathLength = static_cast <unsigned char> (pSettingsFile ->sPathToLogFile .size());
       newSettingsFile .write
               ( reinterpret_cast<char*>(&cLogFilePathLength), sizeof(cLogFilePathLength) );

#if _WIN32
       newSettingsFile .write
               ( reinterpret_cast<char*>(  const_cast <wchar_t*>(pSettingsFile ->sPathToLogFile .c_str())  ), cLogFilePathLength * 2 );
#elif __linux__
       newSettingsFile .write
               ( reinterpret_cast<char*>(  const_cast <char*>(pSettingsFile ->sPathToLogFile .c_str())  ), cLogFilePathLength );
#endif
    }

    // NEW SETTINGS GO HERE
    // + don't forget to update "readSettings()"



    if ( settingsFile .is_open() )
    {
        // Close files and rename the new file.

        settingsFile    .close();
        newSettingsFile .close();

#if _WIN32
        _wremove( sPathToOldSettings .c_str() );

        _wrename( sPathToNewSettings .c_str(), sPathToOldSettings .c_str() );
#elif __linux__
        unlink(sPathToOldSettings .c_str());

        rename(sPathToNewSettings .c_str(), sPathToOldSettings .c_str());
#endif
    }
    else
    {
        newSettingsFile .close();
    }




    // Update our 'pCurrentSettingsFile' to the new settings

    mtxUpdateSettings .lock();


    if (pCurrentSettingsFile)
    {
        delete pCurrentSettingsFile;
        pCurrentSettingsFile = nullptr;
    }

    pCurrentSettingsFile = pSettingsFile;


    mtxUpdateSettings .unlock();
}

SettingsFile *SettingsManager::getCurrentSettings()
{
    mtxUpdateSettings .lock();
    mtxUpdateSettings .unlock();

    return pCurrentSettingsFile;
}

SettingsFile *SettingsManager::readSettings()
{
#if _WIN32
    // Get the path to the Documents folder.

    TCHAR   my_documents[MAX_PATH];
    HRESULT result = SHGetFolderPathW( nullptr, CSIDL_PERSONAL, nullptr, SHGFP_TYPE_CURRENT, my_documents );

    if (result != S_OK)
    {
        pMainWindow ->showMessageBox(true, L"ServerService::readSettings::SHGetFolderPathW() failed. "
                                           "Can't open the settings file.");

        return nullptr;
    }

    // Open the settings file.

    std::wstring adressToSettings = std::wstring(my_documents);
    adressToSettings             += L"\\" + std::wstring(SETTINGS_NAME);
#elif __linux__
    char* pHomeDir = NULL;
    pHomeDir = getenv("HOME");

    std::string adressToSettings = std::string(pHomeDir);

    if (pHomeDir)
    {
        adressToSettings             += "/" + std::string(SETTINGS_NAME);
    }
    else
    {
#if _WIN32
        pMainWindow ->showMessageBox(true, L"ServerService::readSettings() failed. "
                                           "Can't get path to HOME.");
#elif __linux__
        pMainWindow ->showMessageBox(true, u"ServerService::readSettings() failed. "
                                           "Can't get path to HOME.");
#endif

        return nullptr;
    }
#endif




    // Create settings file object

    SettingsFile* pSettingsFile = new SettingsFile();


    std::ifstream settingsFile (adressToSettings, std::ios::binary);

    // Get file size.
    settingsFile .seekg(0, std::ios::end);
    long long iFileSize = settingsFile .tellg();
    settingsFile .seekg(0, std::ios::beg);

    long long iPos = 0;

    if ( settingsFile .is_open() )
    {
        // Read the settings.



        // Read port
        settingsFile .read( reinterpret_cast <char*> (&pSettingsFile ->iPort), sizeof(pSettingsFile ->iPort) );
        iPos += sizeof(pSettingsFile ->iPort);




        // Settings may end here
        if (iPos >= iFileSize)
        {
            settingsFile .close();

            saveSettings(pSettingsFile);
        }



        // Read allow HTML in messages
        char cAllowHTMLInMessages;
        settingsFile .read( reinterpret_cast <char*> (&cAllowHTMLInMessages), sizeof(cAllowHTMLInMessages) );
        iPos += sizeof(cAllowHTMLInMessages);

        pSettingsFile ->bAllowHTMLInMessages = cAllowHTMLInMessages;




        // Read password to join
        unsigned char cPasswordToJoinLength = 0;
        settingsFile .read( reinterpret_cast <char*> (&cPasswordToJoinLength), sizeof(cPasswordToJoinLength) );
        iPos += sizeof(cPasswordToJoinLength);

        S16Char vBuffer[UCHAR_MAX + 1];
        memset(vBuffer, 0, UCHAR_MAX + 1);

        settingsFile .read( reinterpret_cast<char*>( vBuffer ), cPasswordToJoinLength * 2); // because wchar_t is 2 bytes each char
        iPos += cPasswordToJoinLength * 2;

        pSettingsFile ->sPasswordToJoin = vBuffer;


        // Read log
        char cLog = 0;
        settingsFile .read( &cLog, sizeof(cLog) );
        iPos += sizeof(cLog);

        if (cLog)
        {
            pSettingsFile ->bLog = true;


            unsigned char cLogFilePathLength = 0;
            settingsFile .read
                   ( reinterpret_cast<char*>(&cLogFilePathLength), sizeof(cLogFilePathLength) );
            iPos += sizeof(cLogFilePathLength);


#if _WIN32
            memset(vBuffer, 0, UCHAR_MAX + 1);
            settingsFile .read
                   ( reinterpret_cast<char*>( vBuffer ), cLogFilePathLength * 2 );
            iPos += cLogFilePathLength * 2;

            pSettingsFile ->sPathToLogFile = vBuffer;
#elif __linux__
            char vLogBuffer[UCHAR_MAX + 1];
            memset(vLogBuffer, 0, UCHAR_MAX + 1);
            settingsFile .read
                   ( reinterpret_cast<char*>( vLogBuffer ), cLogFilePathLength );
            iPos += cLogFilePathLength;

            pSettingsFile ->sPathToLogFile = vLogBuffer;
#endif
        }


        settingsFile .close();
    }
    else
    {
        // The settings file does not exist.
        // Create one and write default settings.


        saveSettings(pSettingsFile);
    }



    return pSettingsFile;
}
