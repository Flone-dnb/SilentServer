#include "logmanager.h"


// STL
#include <fstream>
#include <thread>
#include <ctime>

// Custom
#include "../src/View/MainWindow/mainwindow.h"
#include "../src/Model/SettingsManager/settingsmanager.h"
#include "../src/Model/SettingsManager/SettingsFile.h"

// Other
#include <Windows.h>



// ------------------------------------------------------------------------------------------------
// ------------------------------------------------------------------------------------------------
// ------------------------------------------------------------------------------------------------



LogManager::LogManager(MainWindow* pMainWindow, SettingsManager* pSettingsManager)
{
    this ->pMainWindow      = pMainWindow;
    this ->pSettingsManager = pSettingsManager;


    eraseTempFile();


    if (pSettingsManager ->getCurrentSettings() ->bLog)
    {
        std::ofstream logFile (pSettingsManager ->getCurrentSettings() ->sPathToLogFile, std::ios::app);

        if ( logFile .is_open() )
        {
            logFile .close();


            _wremove( pSettingsManager ->getCurrentSettings() ->sPathToLogFile .c_str() );
        }
    }
}


void LogManager::printAndLog(std::string sText, bool bEmitSignal)
{
    pMainWindow ->printOutput(sText, bEmitSignal);


    if ( pSettingsManager ->getCurrentSettings() ->bLog )
    {
        std::thread tLog(&LogManager::logThread, this, sText);
        tLog .detach();
    }
}

void LogManager::archiveText(wchar_t *pText, size_t iWChars)
{
    std::thread tArchive (&LogManager::archiveThread, this, pText, iWChars);
    tArchive .detach();
}

void LogManager::showOldText()
{
    std::thread tShow (&LogManager::showTextThread, this);
    tShow .detach();
}

void LogManager::stop()
{
    mtxLogWrite .lock();
    mtxLogWrite .unlock();


    eraseTempFile();
}

void LogManager::logThread(std::string sText)
{
    mtxLogWrite .lock();


    std::ofstream logFile (pSettingsManager ->getCurrentSettings() ->sPathToLogFile, std::ios::app);



    // Get time and date.
    time_t tTimeNow = time(nullptr);

    tm now;
    ::localtime_s(&now, &tTimeNow);
    std::string sTimeString = "";

    sTimeString += "[" + std::to_string(now .tm_year + 1900) + '.' + std::to_string(now .tm_mon + 1)
                   + '.' + std::to_string(now .tm_mday) + "] ";

    sTimeString += std::to_string(now .tm_hour) + ':' + std::to_string(now .tm_min) + '.';
    sTimeString += "\n";


    logFile << sTimeString;
    logFile << sText;
    logFile << "\n";


    logFile .close();


    mtxLogWrite .unlock();
}

void LogManager::archiveThread(wchar_t *pText, size_t iWChars)
{
    mtxLogWrite .lock();


    // Prepare the buffer.

    wchar_t vBuffer[MAX_PATH + 1];
    memset(vBuffer, 0, MAX_PATH + 1);




    // Get the path to the TEMP.

    if ( GetTempPathW(MAX_PATH, vBuffer) == 0 )
    {
        mtxLogWrite .unlock();

        printAndLog("Can't get the path to the TEMP directory.\n", true);

        delete[] pText;

        return;
    }




    // Convert the buffer to the std::wstring and add filename.

    std::wstring sPathToOldFile(vBuffer);

    sPathToOldFile += L"\\";
    sPathToOldFile += TEMP_FILE_NAME;




    // Write temp text to the new file.

    std::wstring sPathToNewFile = sPathToOldFile + L"~";
    std::ofstream newTempFile(sPathToNewFile, std::ios::binary);

    const int iOneReadLengthInBytes = 1024 * 128;

    char vReadBuffer[iOneReadLengthInBytes];

    size_t iCurrentTextPosInBytes = 0;
    size_t iStringSizeInBytes     = iWChars * 2;

    newTempFile .write( reinterpret_cast <char*>(&iWChars), sizeof(iWChars) );


    while (iCurrentTextPosInBytes < iStringSizeInBytes)
    {
        if ( (iCurrentTextPosInBytes + iOneReadLengthInBytes) < iStringSizeInBytes )
        {
            // Not the last read.

            std::memcpy(vReadBuffer, reinterpret_cast <char*>(pText) + iCurrentTextPosInBytes, iOneReadLengthInBytes);

            newTempFile .write( vReadBuffer, iOneReadLengthInBytes );

            iCurrentTextPosInBytes += iOneReadLengthInBytes;
        }
        else
        {
            // Last read.

            size_t iLeftBytes = iStringSizeInBytes - iCurrentTextPosInBytes;

            std::memcpy(vReadBuffer, reinterpret_cast <char*>(pText) + iCurrentTextPosInBytes, iLeftBytes);

            newTempFile .write( vReadBuffer, static_cast <long long>(iLeftBytes) );

            iCurrentTextPosInBytes += iLeftBytes;
        }
    }




    // Check if an old temp file exists.

    std::ifstream oldTempFile(sPathToOldFile, std::ios::binary);

    if ( oldTempFile .is_open() )
    {
        // Copy this old file to the new file.

        long long iOldFileSize = 0;
        oldTempFile .seekg(0, std::ios::end);
        iOldFileSize = oldTempFile .tellg();
        oldTempFile .seekg(0, std::ios::beg);

        long long iCurrentPos = 0;


        while ( iCurrentPos < iOldFileSize )
        {
            if ( (iCurrentPos + iOneReadLengthInBytes) < iOldFileSize )
            {
                // Not the last read.

                oldTempFile .read(vReadBuffer, iOneReadLengthInBytes);
                newTempFile .write(vReadBuffer, iOneReadLengthInBytes);

                iCurrentPos += iOneReadLengthInBytes;
            }
            else
            {
                // Last read.

                long long iLeftBytes = iOldFileSize - iCurrentPos;

                oldTempFile .read(vReadBuffer, iLeftBytes);
                newTempFile .write(vReadBuffer, iLeftBytes);

                iCurrentPos += iLeftBytes;
            }
        }


        newTempFile .close();
        oldTempFile .close();


        _wremove(sPathToOldFile .c_str());
    }
    else
    {
        newTempFile .close();
    }

    _wrename(sPathToNewFile .c_str(), sPathToOldFile .c_str());


    delete[] pText;


    mtxLogWrite .unlock();
}

void LogManager::showTextThread()
{
    mtxLogWrite .lock();


    // Prepare the buffer.

    wchar_t vBuffer[MAX_PATH + 1];
    memset(vBuffer, 0, MAX_PATH + 1);




    // Get the path to the TEMP.

    if ( GetTempPathW(MAX_PATH, vBuffer) == 0 )
    {
        mtxLogWrite .unlock();

        printAndLog("Can't get the path to the TEMP directory.\n", true);

        return;
    }




    // Convert the buffer to the std::wstring and add filename.

    std::wstring sPathToOldFile(vBuffer);

    sPathToOldFile += L"\\";
    sPathToOldFile += TEMP_FILE_NAME;




    // Check if it exists.

    std::ifstream tempFile (sPathToOldFile, std::ios::binary);

    if (!tempFile .is_open())
    {
        mtxLogWrite .unlock();

        return;
    }




    // Read text to show.

    size_t iSizeOfTextInWChars = 0;

    tempFile .read(reinterpret_cast <char*>(&iSizeOfTextInWChars), sizeof(iSizeOfTextInWChars));

    wchar_t* pText = new wchar_t[iSizeOfTextInWChars + 1];
    memset(pText, 0, (iSizeOfTextInWChars * sizeof(wchar_t)) + sizeof(wchar_t));

    tempFile .read( reinterpret_cast <char*>(pText), static_cast <long long>(iSizeOfTextInWChars)
                    * static_cast <long long>(sizeof(wchar_t)) );

    tempFile .close();




    // Get length of this file.

    long long iOldFileSize = 0;

    tempFile .open(sPathToOldFile, std::ios::binary);

    tempFile .seekg(0, std::ios::end);

    iOldFileSize = tempFile .tellg();

    tempFile .seekg(static_cast <long long> (sizeof(iSizeOfTextInWChars)) + static_cast <long long>(iSizeOfTextInWChars) * 2);




    // Pass to MainWindow

    pMainWindow ->showOldText(pText);




    // Move left text to the new file.

    std::wstring sPathToNewFile = sPathToOldFile + L"~";
    std::ofstream newTempFile(sPathToNewFile, std::ios::binary);

    const int iOneReadLengthBytes = 1024 * 128;

    char vReadBuffer[iOneReadLengthBytes];

    long long iCurrentTextPos = static_cast <long long>(sizeof(iSizeOfTextInWChars))
            + static_cast <long long>(iSizeOfTextInWChars) * 2;


    bool bFirstRead = true;

    while (iCurrentTextPos < iOldFileSize)
    {
        if ( (iCurrentTextPos + iOneReadLengthBytes) < iOldFileSize )
        {
            // Not the last read.

            tempFile .read(vReadBuffer, iOneReadLengthBytes);
            newTempFile .write(vReadBuffer, iOneReadLengthBytes);

            iCurrentTextPos += iOneReadLengthBytes;
        }
        else
        {
            // Last read.

            long long iLeftBytes = iOldFileSize - iCurrentTextPos;

            tempFile .read(vReadBuffer, iLeftBytes);
            newTempFile .write(vReadBuffer, iLeftBytes);

            iCurrentTextPos += iLeftBytes;
        }

        bFirstRead = false;
    }

    tempFile .close();
    newTempFile .close();

    _wremove(sPathToOldFile .c_str());

    if (bFirstRead)
    {
        _wremove(sPathToNewFile .c_str());
    }
    else
    {
        _wrename(sPathToNewFile .c_str(), sPathToOldFile .c_str());
    }


    mtxLogWrite .unlock();
}

void LogManager::eraseTempFile()
{
    // Erase the temp file.

    // Prepare the buffer.

    wchar_t vBuffer[MAX_PATH + 1];
    memset(vBuffer, 0, MAX_PATH + 1);




    // Get the path to the TEMP.

    if ( GetTempPathW(MAX_PATH, vBuffer) == 0 )
    {
        printAndLog("Can't get the path to the TEMP directory.\n", true);

        return;
    }




    // Convert the buffer to the std::wstring and add filename.

    std::wstring sPathToOldFile(vBuffer);

    sPathToOldFile += L"\\";
    sPathToOldFile += TEMP_FILE_NAME;



    // Check if it exists.

    std::ifstream tempFile(sPathToOldFile);


    if (tempFile .is_open())
    {
        tempFile .close();

        _wremove( sPathToOldFile .c_str() );
    }
}
