TARGET = tst_allcursors
TEMPLATE = app
QT = core gui widgets

SOURCES += main.cpp\
        mainwindow.cpp

HEADERS  += mainwindow.h

FORMS    += mainwindow.ui
DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0
