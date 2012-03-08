CONFIG += testcase parallel_test
TARGET = ../tst_qlogging
QT = core testlib
SOURCES = ../tst_qlogging.cpp

load(testcase) # for target.path and installTestHelperApp()
installTestHelperApp("../app/app",app,app)
