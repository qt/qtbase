CONFIG += testcase
CONFIG += parallel_test
TARGET = tst_qhttpnetworkconnection
SOURCES  += tst_qhttpnetworkconnection.cpp
requires(contains(QT_CONFIG,private_tests))

QT = core-private network-private testlib
