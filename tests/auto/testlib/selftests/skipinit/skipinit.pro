SOURCES += tst_skipinit.cpp
QT = core testlib

mac:CONFIG -= app_bundle
CONFIG -= debug_and_release_target

TARGET = skipinit

include($$QT_SOURCE_TREE/src/testlib/selfcover.pri)
