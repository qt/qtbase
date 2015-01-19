CONFIG += testcase
contains(QT_CONFIG, c++14): CONFIG += c++14
TARGET = tst_qsizepolicy

QT += widgets widgets-private testlib

SOURCES += tst_qsizepolicy.cpp
