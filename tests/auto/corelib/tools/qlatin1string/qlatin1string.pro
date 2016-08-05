CONFIG += testcase parallel_test
TARGET = tst_qlatin1string
QT = core testlib
SOURCES = tst_qlatin1string.cpp
DEFINES += QT_NO_CAST_TO_ASCII
qtConfig(c++11): CONFIG += c++11
qtConfig(c++14): CONFIG += c++14

DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0
