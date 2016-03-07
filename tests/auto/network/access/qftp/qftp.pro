CONFIG += testcase
TARGET = tst_qftp
SOURCES  += tst_qftp.cpp

requires(contains(QT_CONFIG,private_tests))
QT = core network network-private testlib
