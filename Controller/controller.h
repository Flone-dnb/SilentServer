#pragma once


class MainWindow;
class ServerService;


class Controller
{

public:

    Controller(MainWindow* pMainWindow);

    void start();

    void stop();

    ~Controller();

private:

    ServerService* pServerService;
};
