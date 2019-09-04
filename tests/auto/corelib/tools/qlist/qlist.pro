CONFIG += testcase
TARGET = tst_qlist
QT = core testlib
qtConfig(c++14): CONFIG += c++14
qtConfig(c++1z): CONFIG += c++1z
SOURCES = $$PWD/tst_qlist.cpp
