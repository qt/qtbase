SOURCES += tst_deleteLater.cpp
QT = core testlib

CONFIG -= app_bundle
CONFIG -= debug_and_release_target

TARGET = deleteLater

include($$QT_SOURCE_TREE/src/testlib/selfcover.pri)
