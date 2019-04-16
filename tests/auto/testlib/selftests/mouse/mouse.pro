SOURCES += tst_mouse.cpp
QT += testlib testlib-private gui gui-private

mac:CONFIG -= app_bundle
CONFIG -= debug_and_release_target

TARGET = mouse

include($$QT_SOURCE_TREE/src/testlib/selfcover.pri)
