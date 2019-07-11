CONFIG += testcase
TARGET = tst_qglobal
QT = core testlib
SOURCES = tst_qglobal.cpp qglobal.c
qtConfig(c++11): CONFIG += c++11
qtConfig(c++14): CONFIG += c++14
qtConfig(c++1z): CONFIG += c++1z
