CONFIG += testcase
TARGET = tst_qstringapisymmetry
QT = core testlib
SOURCES = tst_qstringapisymmetry.cpp
qtConfig(c++14): CONFIG += c++14
qtConfig(c++1z): CONFIG += c++1z
