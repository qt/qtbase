SOURCES += tst_fetchbogus.cpp
QT = core testlib

mac:CONFIG -= app_bundle
CONFIG -= debug_and_release_target

TARGET = fetchbogus

include($$QT_SOURCE_TREE/src/testlib/selfcover.pri)
