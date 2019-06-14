#pragma once

#include <string>

class MainWindow;
class ServerService;


class Controller
{

public:

    Controller(MainWindow* pMainWindow);

    std::string getServerVersion();

    void start();

    void stop();

    ~Controller();

private:

    ServerService* pServerService;

    bool bServerStarted;
};
