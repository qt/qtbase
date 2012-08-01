SOURCES += tst_commandlinedata.cpp
QT = core testlib

mac:CONFIG -= app_bundle
CONFIG -= debug_and_release_target

TARGET = commandlinedata
DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0
