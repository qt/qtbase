CONFIG += testcase parallel_test
CONFIG -= app_bundle debug_and_release_target
TARGET = ../tst_qlogging
QT = core testlib
SOURCES = ../tst_qlogging.cpp

load(testcase) # for target.path and installTestHelperApp()
installTestHelperApp("../app/app",app,app)
