SOURCES += tst_expectfail.cpp
QT = core testlib

mac:CONFIG -= app_bundle
CONFIG -= debug_and_release_target

TARGET = expectfail
DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0
