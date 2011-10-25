CONFIG += testcase
TARGET = tst_qstringmatcher
SOURCES  += tst_qstringmatcher.cpp
QT = core testlib
DEFINES += QT_NO_CAST_TO_ASCII

CONFIG += parallel_test
