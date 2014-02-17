CONFIG += testcase
CONFIG += parallel_test
TARGET = tst_qhttpnetworkreply
SOURCES  += tst_qhttpnetworkreply.cpp
requires(contains(QT_CONFIG,private_tests))

QT = core-private network-private testlib
