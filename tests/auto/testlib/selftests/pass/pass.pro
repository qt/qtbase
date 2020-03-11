SOURCES += tst_pass.cpp
QT = core testlib

macos:CONFIG -= app_bundle
CONFIG -= debug_and_release_target

TARGET = pass

include($$QT_SOURCE_TREE/src/testlib/selfcover.pri)
