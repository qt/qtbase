CONFIG += testcase parallel_test
CONFIG -= app_bundle debug_and_release_target
TARGET = ../tst_qlogging
QT = core testlib
SOURCES = ../tst_qlogging.cpp

TEST_HELPER_INSTALLS = ../app/app
DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0
