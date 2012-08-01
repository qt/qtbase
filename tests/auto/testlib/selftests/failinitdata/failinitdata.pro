SOURCES += tst_failinitdata.cpp
QT = core testlib

mac:CONFIG -= app_bundle
CONFIG -= debug_and_release_target

TARGET = failinitdata
DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0
