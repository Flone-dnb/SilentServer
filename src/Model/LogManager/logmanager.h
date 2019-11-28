#pragma once


// STL
#include <string>
#include <mutex>



class MainWindow;
class SettingsManager;



#define TEMP_FILE_NAME L"SilentServerTempText.temp~"


// ------------------------------------------------------------------------------------------------
// ------------------------------------------------------------------------------------------------
// ------------------------------------------------------------------------------------------------



class LogManager
{
public:

    LogManager(MainWindow* pMainWindow, SettingsManager* pSettingsManager);


    void  printAndLog   (std::string sText, bool bEmitSignal = false);
    void  archiveText   (wchar_t* pText, size_t iWChars);
    void  showOldText   ();
    void  eraseTempFile ();



    void  stop          ();

private:

    void  logThread     (std::string sText);
    void  archiveThread (wchar_t* pText, size_t iWChars);
    void  showTextThread();



    MainWindow*      pMainWindow;
    SettingsManager* pSettingsManager;


    std::mutex       mtxLogWrite;
};
