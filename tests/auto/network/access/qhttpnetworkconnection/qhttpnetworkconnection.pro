CONFIG += testcase
TARGET = tst_qhttpnetworkconnection
SOURCES  += tst_qhttpnetworkconnection.cpp
requires(qtConfig(private_tests))

QT = core-private network-private testlib
