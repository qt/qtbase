CONFIG += testcase
TARGET  = ../tst_qpluginloader
QT = core testlib
SOURCES = ../tst_qpluginloader.cpp
HEADERS = ../theplugin/plugininterface.h
DEFINES += SRCDIR=\\\"$$PWD/../\\\"

win32 {
    CONFIG(debug, debug|release) {
        TARGET = ../../debug/tst_qpluginloader
    } else {
        TARGET = ../../release/tst_qpluginloader
    }
}

wince* {
   addFiles.files = $$OUT_PWD/../bin/*.dll
   addFiles.path = bin
   DEPLOYMENT += addFiles
}
