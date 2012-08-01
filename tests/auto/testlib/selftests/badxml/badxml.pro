SOURCES += tst_badxml.cpp
QT = core-private testlib

mac:CONFIG -= app_bundle
CONFIG -= debug_and_release_target

TARGET = badxml
DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0
