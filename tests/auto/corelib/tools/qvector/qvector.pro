CONFIG += testcase parallel_test
contains(QT_CONFIG, c++11):CONFIG += c++11
TARGET = tst_qvector
QT = core testlib
SOURCES = $$PWD/tst_qvector.cpp
DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0
