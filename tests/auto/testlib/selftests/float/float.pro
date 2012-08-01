SOURCES += tst_float.cpp
QT = core testlib

mac:CONFIG -= app_bundle
CONFIG -= debug_and_release_target

TARGET = float
DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0
