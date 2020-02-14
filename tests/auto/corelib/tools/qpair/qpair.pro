CONFIG += testcase
TARGET = tst_qpair
QT = core testlib
SOURCES = tst_qpair.cpp

contains(QT_CONFIG, c++2a): CONFIG += c++2a
