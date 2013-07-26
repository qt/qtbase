CONFIG += testcase parallel_test
contains(QT_CONFIG, c++11):CONFIG += c++11
TARGET = tst_qscopedpointer
QT = core testlib
SOURCES = tst_qscopedpointer.cpp
DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0
