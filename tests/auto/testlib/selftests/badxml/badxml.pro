SOURCES += tst_badxml.cpp
QT = core-private testlib

mac:CONFIG -= app_bundle
CONFIG -= debug_and_release_target

TARGET = badxml

include($$QT_SOURCE_TREE/src/testlib/selfcover.pri)
