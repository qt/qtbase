CONFIG += testcase
TARGET = tst_qvarlengtharray
QT = core testlib
qtConfig(c++14): CONFIG += c++14
qtConfig(c++1z): CONFIG += c++1z
SOURCES = tst_qvarlengtharray.cpp
