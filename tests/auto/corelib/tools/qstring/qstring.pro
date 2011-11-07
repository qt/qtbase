CONFIG += testcase parallel_test
TARGET = tst_qstring
QT = core testlib
SOURCES = tst_qstring.cpp
DEFINES += QT_NO_CAST_TO_ASCII
contains(QT_CONFIG,icu):DEFINES += QT_USE_ICU
