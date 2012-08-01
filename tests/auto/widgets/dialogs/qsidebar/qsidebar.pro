CONFIG += testcase
CONFIG += parallel_test

QT += core-private
QT += widgets widgets-private testlib
SOURCES		+= tst_qsidebar.cpp 
TARGET		= tst_qsidebar
DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0
