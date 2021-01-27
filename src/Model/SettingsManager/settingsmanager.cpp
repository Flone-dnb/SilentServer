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
    this->pMainWindow = pMainWindow;


    pCurrentSettingsFile = nullptr;
    pCurrentSettingsFile = readSettings();
}

SettingsManager::~SettingsManager()
{
    if (pCurrentSettingsFile)
    {
        delete pCurrentSettingsFile;
    }
}







void SettingsManager::saveSettings(SettingsFile *pSettingsFile)
{
#if _WIN32
    // Get the path to the Documents folder.

    TCHAR   my_documents[MAX_PATH];
    HRESULT result = SHGetFolderPathW( nullptr, CSIDL_PERSONAL, nullptr, SHGFP_TYPE_CURRENT, my_documents );

    if (result != S_OK)
    {
        pMainWindow->showMessageBox(true, u"ServerService::showSettings::SHGetFolderPathW() failed. "
                                           "Can't open the settings file.");

        delete pSettingsFile;

        return;
    }

    if (pSettingsFile == pCurrentSettingsFile)
    {
        // Copy, because we will 'delete pCurrentSettingsFile' in the end of the function.
        // and do 'pCurrentSettingsFile = pSettingsFile'.
        pSettingsFile = new SettingsFile(*pCurrentSettingsFile);
    }


    // Open the settings file.

    std::wstring temp1 = std::wstring(my_documents);
    std::u16string sPathToOldSettings(temp1.begin(), temp1.end());
    std::u16string sPathToNewSettings(temp1.begin(), temp1.end());

    sPathToOldSettings  += u"\\" + std::u16string(SETTINGS_NAME);
    sPathToNewSettings  += u"\\" + std::u16string(SETTINGS_NAME) + u"~";
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
        pMainWindow->showMessageBox(true, u"ServerService::showSettings() failed. "
                                           "Can't get path to HOME.");
#elif __linux__
        pMainWindow->showMessageBox(true, u"ServerService::showSettings() failed. "
                                           "Can't get path to HOME.");
#endif

        delete pSettingsFile;

        return;
    }
#endif

#if _WIN32
    std::wstring path (sPathToOldSettings.begin(), sPathToOldSettings.end());
    std::ifstream settingsFile (path, std::ios::binary);
#elif __linux__
    std::ifstream settingsFile (sPathToOldSettings, std::ios::binary);
#endif
    std::ofstream newSettingsFile;

    if ( settingsFile.is_open() )
    {
#if _WIN32
        std::wstring path1 (sPathToNewSettings.begin(), sPathToNewSettings.end());
        newSettingsFile.open(path1);
#elif __linux__
        newSettingsFile.open(sPathToNewSettings);
#endif
    }
    else
    {
#if _WIN32
        std::wstring path1 (sPathToOldSettings.begin(), sPathToOldSettings.end());
        newSettingsFile.open(path1);
#elif __linux__
        newSettingsFile.open(sPathToOldSettings);
#endif
    }




    // Write new setting to the new file.

    // Save magic number.

    unsigned int iMagicNumber = SETTINGS_FILE_MAGIC_NUMBER;
    newSettingsFile.write
            ( reinterpret_cast <char*> (&iMagicNumber), sizeof(iMagicNumber) );

    // Save settings file version.
    unsigned short iVersion = SETTINGS_FILE_VERSION;
    newSettingsFile.write
            ( reinterpret_cast <char*> (&iVersion), sizeof(iVersion) );

    // Server port
    newSettingsFile.write
            ( reinterpret_cast <char*> (&pSettingsFile->iPort), sizeof(pSettingsFile->iPort) );

    // Allow HTML in messages
    char cAllowHTMLInMessages = pSettingsFile->bAllowHTMLInMessages;
    newSettingsFile.write
            ( &cAllowHTMLInMessages, sizeof(cAllowHTMLInMessages) );

    // Password to join
    unsigned char cPasswordToJoinLength = static_cast <unsigned char> ( pSettingsFile->sPasswordToJoin.size() );
    newSettingsFile.write
            ( reinterpret_cast<char*>(&cPasswordToJoinLength), sizeof(cPasswordToJoinLength) );

    newSettingsFile.write
            ( reinterpret_cast<char*>(  const_cast <char16_t*>(pSettingsFile->sPasswordToJoin.c_str())  ), cPasswordToJoinLength * 2 );

    // Log
    char cLog = pSettingsFile->bLog;
    newSettingsFile.write
            ( &cLog, sizeof(cLog) );

    if (pSettingsFile->bLog)
    {
       unsigned char cLogFilePathLength = static_cast <unsigned char> (pSettingsFile->sPathToLogFile.size());
       newSettingsFile.write
               ( reinterpret_cast<char*>(&cLogFilePathLength), sizeof(cLogFilePathLength) );

#if _WIN32
       newSettingsFile.write
               ( reinterpret_cast<char*>(  const_cast <char16_t*>(pSettingsFile->sPathToLogFile.c_str())  ), cLogFilePathLength * 2 );
#elif __linux__
       newSettingsFile.write
               ( reinterpret_cast<char*>(  const_cast <char*>(pSettingsFile->sPathToLogFile.c_str())  ), cLogFilePathLength );
#endif
    }

    // Save room settings
    std::vector<std::string> vRoomNames = pMainWindow->getRoomNames();

    unsigned char cRoomCount = static_cast<unsigned char>( vRoomNames.size() );

    newSettingsFile.write( reinterpret_cast<char*>(&cRoomCount), sizeof(unsigned char) );

    for (unsigned char i = 0; i < cRoomCount; i++)
    {
        // Write room name size.

        char cRoomNameSize = static_cast<char>( vRoomNames[i].size() );
        newSettingsFile.write( &cRoomNameSize, sizeof(char) );



        // Write room name.

        newSettingsFile.write( vRoomNames[i].c_str(), vRoomNames[i].size() );



        // Write room password size.

        char cRoomPassSize = static_cast<char>( pMainWindow->getRoomPassword(i).size() );
        newSettingsFile.write( &cRoomPassSize, sizeof(char) );



        // Write room password.

        std::u16string sRoomPassword = pMainWindow->getRoomPassword(i);
        newSettingsFile.write( reinterpret_cast<char*>(const_cast<char16_t*>(sRoomPassword.c_str())), sRoomPassword.size() * 2 );


        // Write room max users.

        unsigned short int iMaxUsers = pMainWindow->getRoomMaxUsers(i);
        newSettingsFile.write( reinterpret_cast<char*>(&iMaxUsers), sizeof(iMaxUsers) );


        // Write room message size.

        unsigned short int iRoomMessageSize = pMainWindow->getRoomMessage(i).length() * 2;
        newSettingsFile.write( reinterpret_cast<char*>(&iRoomMessageSize), sizeof(iRoomMessageSize) );


        // Write room message.

        std::u16string sRoomMessage = pMainWindow->getRoomMessage(i);
        newSettingsFile.write( reinterpret_cast<const char*>(sRoomMessage.c_str()), iRoomMessageSize );
    }

    // NEW SETTINGS GO HERE
    // + don't forget to update "readSettings()"



    if ( settingsFile.is_open() )
    {
        // Close files and rename the new file.

        settingsFile   .close();
        newSettingsFile.close();

#if _WIN32
        std::wstring path1 (sPathToOldSettings.begin(), sPathToOldSettings.end());
        _wremove( path1.c_str() );

        std::wstring path2 (sPathToNewSettings.begin(), sPathToNewSettings.end());
        _wrename( path2.c_str(), path1.c_str() );
#elif __linux__
        unlink(sPathToOldSettings.c_str());

        rename(sPathToNewSettings.c_str(), sPathToOldSettings.c_str());
#endif
    }
    else
    {
        newSettingsFile.close();
    }




    // Update our 'pCurrentSettingsFile' to the new settings

    mtxUpdateSettings.lock();


    if (pCurrentSettingsFile)
    {
        delete pCurrentSettingsFile;
        pCurrentSettingsFile = nullptr;
    }

    pCurrentSettingsFile = pSettingsFile;


    mtxUpdateSettings.unlock();
}

SettingsFile *SettingsManager::getCurrentSettings()
{
    mtxUpdateSettings.lock();
    mtxUpdateSettings.unlock();

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
        pMainWindow->showMessageBox(true, u"ServerService::readSettings::SHGetFolderPathW() failed. "
                                           "Can't open the settings file.");

        return nullptr;
    }

    // Open the settings file.


    std::wstring temp = std::wstring(my_documents);
    std::u16string adressToSettings(temp.begin(), temp.end());
    adressToSettings             += u"\\" + std::u16string(SETTINGS_NAME);
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
        pMainWindow->showMessageBox(true, u"ServerService::readSettings() failed. "
                                           "Can't get path to HOME.");
#elif __linux__
        pMainWindow->showMessageBox(true, u"ServerService::readSettings() failed. "
                                           "Can't get path to HOME.");
#endif

        return nullptr;
    }
#endif




    // Create settings file object

    SettingsFile* pSettingsFile = new SettingsFile();


#if _WIN32
    std::wstring path (adressToSettings.begin(), adressToSettings.end());
    std::ifstream settingsFile (path, std::ios::binary);
#elif __linux__
    std::ifstream settingsFile (adressToSettings, std::ios::binary);
#endif

    // Get file size.
    settingsFile.seekg(0, std::ios::end);
    long long iFileSize = settingsFile.tellg();
    settingsFile.seekg(0, std::ios::beg);

    long long iPos = 0;

    if ( settingsFile.is_open() )
    {
        // Read the settings.


        unsigned int iMagicNumber = SETTINGS_FILE_MAGIC_NUMBER;
        settingsFile.read( reinterpret_cast <char*> (&iMagicNumber), sizeof(iMagicNumber) );
        iPos += sizeof(iMagicNumber);

        if (iMagicNumber != SETTINGS_FILE_MAGIC_NUMBER)
        {
            if (pCurrentSettingsFile)
            {
                pMainWindow->showMessageBox(true, u"Found old version settings file. The settings file will be deleted.", true);
            }
            else
            {
                pMainWindow->showMessageBox(true, u"Found old version settings file. The settings file will be deleted.");
            }

            settingsFile.close();

            saveSettings(pSettingsFile);

            return pSettingsFile;
        }

        unsigned short iVersion = 0;
        settingsFile.read( reinterpret_cast <char*> (&iVersion), sizeof(iVersion) );
        iPos += sizeof(iVersion);


        // Read port
        settingsFile.read( reinterpret_cast <char*> (&pSettingsFile->iPort), sizeof(pSettingsFile->iPort) );
        iPos += sizeof(pSettingsFile->iPort);


        // Read allow HTML in messages
        char cAllowHTMLInMessages;
        settingsFile.read( reinterpret_cast <char*> (&cAllowHTMLInMessages), sizeof(cAllowHTMLInMessages) );
        iPos += sizeof(cAllowHTMLInMessages);

        pSettingsFile->bAllowHTMLInMessages = cAllowHTMLInMessages;


        // Read password to join
        unsigned char cPasswordToJoinLength = 0;
        settingsFile.read( reinterpret_cast <char*> (&cPasswordToJoinLength), sizeof(cPasswordToJoinLength) );
        iPos += sizeof(cPasswordToJoinLength);

        char16_t vBuffer[UCHAR_MAX + 1];
        memset(vBuffer, 0, UCHAR_MAX + 1);

        settingsFile.read( reinterpret_cast<char*>( vBuffer ), cPasswordToJoinLength * 2); // because char16_t is 2 bytes each char
        iPos += cPasswordToJoinLength * 2;

        pSettingsFile->sPasswordToJoin = vBuffer;


        // Read log
        char cLog = 0;
        settingsFile.read( &cLog, sizeof(cLog) );
        iPos += sizeof(cLog);

        if (cLog)
        {
            pSettingsFile->bLog = true;


            unsigned char cLogFilePathLength = 0;
            settingsFile.read
                   ( reinterpret_cast<char*>(&cLogFilePathLength), sizeof(cLogFilePathLength) );
            iPos += sizeof(cLogFilePathLength);


#if _WIN32
            memset(vBuffer, 0, UCHAR_MAX + 1);
            settingsFile.read
                   ( reinterpret_cast<char*>( vBuffer ), cLogFilePathLength * 2 );
            iPos += cLogFilePathLength * 2;

            pSettingsFile->sPathToLogFile = vBuffer;
#elif __linux__
            char vLogBuffer[UCHAR_MAX + 1];
            memset(vLogBuffer, 0, UCHAR_MAX + 1);
            settingsFile.read
                   ( reinterpret_cast<char*>( vLogBuffer ), cLogFilePathLength );
            iPos += cLogFilePathLength;

            pSettingsFile->sPathToLogFile = vLogBuffer;
#endif
        }


        // NEW SETTINGS MAY GO HERE
        // DON'T FORGET ABOUT iVersion
        // check the SETTINGS_FILE_VERSION


        unsigned char cRoomCount = 0;

        settingsFile.read( reinterpret_cast<char*>(&cRoomCount), sizeof(unsigned char) );

        if (cRoomCount > 0)
        {
            pMainWindow->clearAllRooms();
        }

        for (unsigned char i = 0; i < cRoomCount; i++)
        {
            // Read room name size.

            char cRoomNameSize = 0;
            settingsFile.read( &cRoomNameSize, sizeof(char) );



            // Read room name.

            char vNameBuffer[MAX_NAME_LENGTH + 1];
            memset(vNameBuffer, 0, MAX_NAME_LENGTH + 1);

            settingsFile.read( vNameBuffer, cRoomNameSize );



            // Read room password size.

            char cRoomPassSize = 0;
            settingsFile.read( &cRoomPassSize, sizeof(char) );



            // Read room password.

            char16_t vPasswordBuffer[MAX_NAME_LENGTH + 1];
            memset(vPasswordBuffer, 0, (MAX_NAME_LENGTH + 1) * 2);
            settingsFile.read( reinterpret_cast<char*>(vPasswordBuffer), cRoomPassSize * 2 );


            // Read room max users.

            unsigned short int iMaxUsers = 0;
            settingsFile.read( reinterpret_cast<char*>(&iMaxUsers), sizeof(unsigned short) );


            // Read room message size.

            unsigned short int iRoomMessageSize = 0;
            settingsFile.read( reinterpret_cast<char*>(&iRoomMessageSize), sizeof(iRoomMessageSize) );


            // Read room message.

            char16_t vRoomMessage[MAX_BUFFER_SIZE];
            memset(vRoomMessage, 0, MAX_BUFFER_SIZE);
            settingsFile.read( reinterpret_cast<char*>(vRoomMessage), iRoomMessageSize );


            // NEW SETTINGS MAY GO HERE
            // DON'T FORGET ABOUT iVersion
            // check the SETTINGS_FILE_VERSION

            pMainWindow->addRoom(vNameBuffer, vPasswordBuffer, iMaxUsers, vRoomMessage);
        }


        // NEW SETTINGS MAY GO HERE
        // DON'T FORGET ABOUT iVersion
        // check the SETTINGS_FILE_VERSION

        settingsFile.close();

        return pSettingsFile;
    }
    else
    {
        // The settings file does not exist.
        // Create one and write default settings.


        saveSettings(pSettingsFile);

        return pSettingsFile;
    }
}
