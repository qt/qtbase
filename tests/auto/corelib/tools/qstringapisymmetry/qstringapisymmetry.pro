CONFIG += testcase
TARGET = tst_qstringapisymmetry
QT = core testlib
SOURCES = tst_qstringapisymmetry.cpp
contains(QT_CONFIG,c++14): CONFIG += c++14
