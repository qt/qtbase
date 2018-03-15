QT += widgets network

TARGET = secureudpserver
TEMPLATE = app

SOURCES += \
        main.cpp \
        mainwindow.cpp \
        server.cpp \
    nicselector.cpp

HEADERS += \
        mainwindow.h \
        server.h \
    nicselector.h

FORMS = mainwindow.ui \
    nicselector.ui

target.path = $$[QT_INSTALL_EXAMPLES]/network/secureudpserver
INSTALLS += target
