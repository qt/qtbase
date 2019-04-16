QT = core testlib
SOURCES += tst_xunit.cpp


mac:CONFIG -= app_bundle
CONFIG -= debug_and_release_target

TARGET = xunit

include($$QT_SOURCE_TREE/src/testlib/selfcover.pri)
