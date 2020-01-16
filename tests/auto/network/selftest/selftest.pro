CONFIG += testcase
TARGET = tst_networkselftest
SOURCES  += tst_networkselftest.cpp

requires(qtConfig(private_tests))
QT = core network network-private testlib
