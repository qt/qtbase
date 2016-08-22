CONFIG += testcase
TARGET = tst_qhttpnetworkreply
SOURCES  += tst_qhttpnetworkreply.cpp
requires(qtConfig(private_tests))

QT = core-private network-private testlib
