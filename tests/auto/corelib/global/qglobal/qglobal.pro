CONFIG += testcase
TARGET = tst_qglobal
QT = core testlib
SOURCES = tst_qglobal.cpp qglobal.c
contains(QT_CONFIG, c++1z): CONFIG += c++1z
