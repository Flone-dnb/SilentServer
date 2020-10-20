QT       += core gui

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = "SilentServer"
TEMPLATE = app

# The following define makes your compiler emit warnings if you use
# any feature of Qt which has been marked as deprecated (the exact warnings
# depend on your compiler). Please consult the documentation of the
# deprecated API in order to know how to port your code away from it.
DEFINES += QT_DEPRECATED_WARNINGS

# You can also make your code fail to compile if you use deprecated APIs.
# In order to do so, uncomment the following line.
# You can also select to disable deprecated APIs only up to a certain version of Qt.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

CONFIG += c++11

SOURCES += \
    ../ext/AES/AES.cpp \
    ../ext/integer/integer.cpp \
    ../src/Model/LogManager/logmanager.cpp \
    ../src/Model/SettingsManager/settingsmanager.cpp \
        ../src/View/AboutWindow/aboutwindow.cpp \
    ../src/View/ChangeRoomNameWindow/changeroomnamewindow.cpp \
    ../src/View/CreateRoomWindow/createroomwindow.cpp \
    ../src/View/CustomList/SListItem/slistitem.cpp \
    ../src/View/CustomList/SListItemRoom/slistitemroom.cpp \
    ../src/View/CustomList/SListItemUser/slistitemuser.cpp \
    ../src/View/CustomList/SListWidget/slistwidget.cpp \
    ../src/View/GlobalMessageWindow/globalmessagewindow.cpp \
        ../src/View/SettingsWindow/settingswindow.cpp \
        ../src/main.cpp \
        ../src/View/MainWindow/mainwindow.cpp \
        ../src/Controller/controller.cpp \
        ../src/Model/ServerService/serverservice.cpp \

HEADERS += \
        ../ext/AES/AES.h \
        ../ext/integer/integer.h \
        ../src/Model/LogManager/logmanager.h \
        ../src/Model/ServerService/UDPPacket.h \
        ../src/Model/SettingsManager/SettingsFile.h \
        ../src/Model/SettingsManager/settingsmanager.h \
        ../src/View/ChangeRoomNameWindow/changeroomnamewindow.h \
        ../src/View/CreateRoomWindow/createroomwindow.h \
        ../src/View/CustomList/SListItem/slistitem.h \
        ../src/View/CustomList/SListItemRoom/slistitemroom.h \
        ../src/View/CustomList/SListItemUser/slistitemuser.h \
        ../src/View/CustomList/SListWidget/slistwidget.h \
        ../src/View/GlobalMessageWindow/globalmessagewindow.h \
        ../src/View/PingColorParams.h \
        ../src/View/AboutWindow/aboutwindow.h \
        ../src/View/MainWindow/mainwindow.h \
        ../src/Model/userstruct.h \
        ../src/Controller/controller.h \
        ../src/Model/ServerService/serverservice.h \
        ../src/View/SettingsWindow/settingswindow.h \
        ../src/Model/net_params.h

FORMS += \
        ../src/View/AboutWindow/aboutwindow.ui \
        ../src/View/ChangeRoomNameWindow/changeroomnamewindow.ui \
        ../src/View/CreateRoomWindow/createroomwindow.ui \
        ../src/View/GlobalMessageWindow/globalmessagewindow.ui \
        ../src/View/MainWindow/mainwindow.ui \
        ../src/View/SettingsWindow/settingswindow.ui

win32
{
        RC_FILE += ../res/file.rc
        OTHER_FILES += ../res/file.rc
}

RESOURCES += ../res/qt_rec_file.qrc

INCLUDEPATH += "../src"
INCLUDEPATH += "../ext"

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target
