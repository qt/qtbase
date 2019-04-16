SOURCES += tst_tuplediagnostics.cpp
QT = core testlib

CONFIG -= app_bundle debug_and_release_target

TARGET = tuplediagnostics

include($$QT_SOURCE_TREE/src/testlib/selfcover.pri)
