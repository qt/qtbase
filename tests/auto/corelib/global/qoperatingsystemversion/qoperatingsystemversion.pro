CONFIG += testcase
TARGET = tst_qoperatingsystemversion
QT = core testlib
SOURCES = tst_qoperatingsystemversion.cpp
qtConfig(c++11): CONFIG += c++11
qtConfig(c++14): CONFIG += c++14
qtConfig(c++1z): CONFIG += c++1z
