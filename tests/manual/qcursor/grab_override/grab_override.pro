TARGET = t_cursors
TEMPLATE = app
QT = core gui widgets

SOURCES += main.cpp\
        mainwindow.cpp

HEADERS  += mainwindow.h

FORMS    += mainwindow.ui

RESOURCES += images.qrc
DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0
