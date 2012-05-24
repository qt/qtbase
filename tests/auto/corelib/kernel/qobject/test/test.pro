CONFIG += testcase console
CONFIG += parallel_test
TARGET = ../tst_qobject
QT = core-private network testlib
SOURCES = ../tst_qobject.cpp

load(testcase) # for target.path and installTestHelperApp()
installTestHelperApp("../signalbug/signalbug",signalbug,signalbug)
