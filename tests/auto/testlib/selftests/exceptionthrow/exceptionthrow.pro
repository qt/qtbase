SOURCES += tst_exceptionthrow.cpp
QT = core testlib

mac:CONFIG -= app_bundle
CONFIG -= debug_and_release_target
CONFIG += exceptions

TARGET = exceptionthrow
DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0
