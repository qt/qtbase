CONFIG += testcase
CONFIG += parallel_test
TARGET = tst_qcomplextext
QT += widgets widgets-private testlib
QT += core-private gui-private
SOURCES  += tst_qcomplextext.cpp
DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0
