SOURCES += tst_cmptest.cpp
QT = core gui testlib

mac:CONFIG -= app_bundle
CONFIG -= debug_and_release_target

TARGET = cmptest
DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0
