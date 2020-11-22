CONFIG += testcase
TARGET = tst_qcompare
QT = core testlib
SOURCES = tst_qcompare.cpp
qtConfig(c++11): CONFIG += c++11
qtConfig(c++14): CONFIG += c++14
qtConfig(c++17): CONFIG += c++17
qtConfig(c++2a): CONFIG += c++2a
