SOURCES += tst_failfetchtype.cpp
QT = core testlib

darwin: CONFIG -= app_bundle
CONFIG -= debug_and_release_target

TARGET = failfetchtype

include($$QT_SOURCE_TREE/src/testlib/selfcover.pri)
