CONFIG += testcase parallel_test
TARGET = tst_qflags
QT = core testlib
SOURCES = tst_qflags.cpp
DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0
contains(QT_CONFIG, c++11): CONFIG += c++11
contains(QT_CONFIG, c++14): CONFIG += c++14
