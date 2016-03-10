CONFIG += testcase
CONFIG += parallel_test
linux: CONFIG += insignificant_test
TARGET = tst_qstatictext
QT += testlib

SOURCES  += tst_qstatictext.cpp

contains(QT_CONFIG, private_tests): QT += core-private gui-private
