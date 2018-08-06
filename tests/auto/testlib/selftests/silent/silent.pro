SOURCES += tst_silent.cpp
QT = core testlib-private

mac:CONFIG -= app_bundle
CONFIG -= debug_and_release_target

TARGET = silent

include($$QT_SOURCE_TREE/src/testlib/selfcover.pri)
