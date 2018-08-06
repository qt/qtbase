SOURCES += tst_multiexec.cpp
QT = core testlib

mac:CONFIG -= app_bundle
CONFIG -= debug_and_release_target

TARGET = multiexec

include($$QT_SOURCE_TREE/src/testlib/selfcover.pri)
