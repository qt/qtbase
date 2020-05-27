CONFIG += testcase
qtConfig(c++11): CONFIG += c++11
qtConfig(c++14): CONFIG += c++14
qtConfig(c++1z): CONFIG += c++1z
TARGET = tst_qlist
QT = core testlib
SOURCES = $$PWD/tst_qlist.cpp
