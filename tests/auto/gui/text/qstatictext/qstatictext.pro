CONFIG += testcase
CONFIG += parallel_test
linux: CONFIG += insignificant_test
TARGET = tst_qstatictext
QT += testlib
QT += core-private gui-private
SOURCES  += tst_qstatictext.cpp
