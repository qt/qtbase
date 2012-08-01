QT = core testlib
SOURCES += tst_xunit.cpp


mac:CONFIG -= app_bundle
CONFIG -= debug_and_release_target

TARGET = xunit
DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0
