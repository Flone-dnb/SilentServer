#include "logmanager.h"


// STL
#include <fstream>
#include <thread>
#include <ctime>

// Custom
#include "../src/View/MainWindow/mainwindow.h"
#include "../src/Model/SettingsManager/settingsmanager.h"
#include "../src/Model/SettingsManager/SettingsFile.h"



// ------------------------------------------------------------------------------------------------
// ------------------------------------------------------------------------------------------------
// ------------------------------------------------------------------------------------------------



LogManager::LogManager(MainWindow* pMainWindow, SettingsManager* pSettingsManager)
{
    this ->pMainWindow      = pMainWindow;
    this ->pSettingsManager = pSettingsManager;


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

void LogManager::stop()
{
    mtxLogWrite .lock();
    mtxLogWrite .unlock();
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
