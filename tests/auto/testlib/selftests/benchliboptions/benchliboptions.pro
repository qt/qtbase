SOURCES += tst_benchliboptions.cpp
QT = core testlib

mac:CONFIG -= app_bundle
CONFIG -= debug_and_release_target

TARGET = benchliboptions

include($$QT_SOURCE_TREE/src/testlib/selfcover.pri)
