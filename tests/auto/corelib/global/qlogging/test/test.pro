CONFIG += testcase parallel_test
CONFIG -= app_bundle debug_and_release_target
contains(QT_CONFIG, c++11): CONFIG += c++11 c++14
TARGET = ../tst_qlogging
QT = core testlib
SOURCES = ../tst_qlogging.cpp

DEFINES += QT_MESSAGELOGCONTEXT
TEST_HELPER_INSTALLS = ../app/app
DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0
