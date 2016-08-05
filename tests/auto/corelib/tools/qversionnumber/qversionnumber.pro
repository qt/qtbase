CONFIG += testcase
qtConfig(c++11): CONFIG += c++11
qtConfig(c++14): CONFIG += c++14
TARGET = tst_qversionnumber
QT = core testlib
SOURCES = tst_qversionnumber.cpp
