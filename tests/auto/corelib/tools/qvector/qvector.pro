CONFIG += testcase
contains(QT_CONFIG, c++11):CONFIG += c++11
TARGET = tst_qvector
QT = core testlib
SOURCES = $$PWD/tst_qvector.cpp
