CONFIG += testcase parallel_test
TARGET = tst_qflags
QT = core testlib
SOURCES = tst_qflags.cpp
contains(QT_CONFIG, c++11): CONFIG += c++11 c++14
