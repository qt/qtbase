SOURCES += tst_alive.cpp
QT = core testlib

mac:CONFIG -= app_bundle
CONFIG -= debug_and_release_target


TARGET = alive

include($$QT_SOURCE_TREE/src/testlib/selfcover.pri)
