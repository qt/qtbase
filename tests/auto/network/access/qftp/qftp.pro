CONFIG += testcase
TARGET = tst_qftp
SOURCES  += tst_qftp.cpp

requires(qtConfig(private_tests))
QT = core network network-private testlib
