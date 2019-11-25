#pragma once


// STL
#include <string>
#include <mutex>



class MainWindow;
class SettingsManager;



// ------------------------------------------------------------------------------------------------
// ------------------------------------------------------------------------------------------------
// ------------------------------------------------------------------------------------------------



class LogManager
{
public:

    LogManager(MainWindow* pMainWindow, SettingsManager* pSettingsManager);


    void printAndLog(std::string sText, bool bEmitSignal = false);


    void stop();

private:

    void logThread(std::string sText);



    MainWindow*      pMainWindow;
    SettingsManager* pSettingsManager;


    std::mutex       mtxLogWrite;
};
