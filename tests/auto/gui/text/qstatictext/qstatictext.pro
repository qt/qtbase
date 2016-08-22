CONFIG += testcase
TARGET = tst_qstatictext
QT += testlib

SOURCES  += tst_qstatictext.cpp

qtConfig(private_tests): QT += core-private gui-private
