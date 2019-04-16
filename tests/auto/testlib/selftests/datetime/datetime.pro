SOURCES += tst_datetime.cpp
QT = core testlib

mac:CONFIG -= app_bundle
CONFIG -= debug_and_release_target

TARGET = datetime

include($$QT_SOURCE_TREE/src/testlib/selfcover.pri)
