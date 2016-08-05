CONFIG += testcase
TARGET = tst_qflags
QT = core testlib
SOURCES = tst_qflags.cpp
qtConfig(c++11): CONFIG += c++11
qtConfig(c++14): CONFIG += c++14
