SOURCES += tst_warnings.cpp
QT = core testlib

mac:CONFIG -= app_bundle
CONFIG -= debug_and_release_target

TARGET = warnings
DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0
