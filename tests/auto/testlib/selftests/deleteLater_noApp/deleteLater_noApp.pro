SOURCES += tst_deleteLater_noApp.cpp
QT = core testlib

CONFIG -= app_bundle
CONFIG -= debug_and_release_target

TARGET = deleteLater_noApp

include($$QT_SOURCE_TREE/src/testlib/selfcover.pri)
