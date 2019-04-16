SOURCES += tst_failcleanup.cpp
QT = core testlib

mac:CONFIG -= app_bundle
CONFIG -= debug_and_release_target

TARGET = failcleanup

include($$QT_SOURCE_TREE/src/testlib/selfcover.pri)
