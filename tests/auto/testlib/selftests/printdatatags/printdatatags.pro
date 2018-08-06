SOURCES += tst_printdatatags.cpp
QT = core testlib

mac:CONFIG -= app_bundle
CONFIG -= debug_and_release_target

TARGET = printdatatags

include($$QT_SOURCE_TREE/src/testlib/selfcover.pri)
