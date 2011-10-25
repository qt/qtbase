CONFIG += testcase
TARGET = tst_qstring
SOURCES  += tst_qstring.cpp

QT = core testlib

DEFINES += QT_NO_CAST_TO_ASCII
CONFIG += parallel_test

contains(QT_CONFIG,icu):DEFINES += QT_USE_ICU
