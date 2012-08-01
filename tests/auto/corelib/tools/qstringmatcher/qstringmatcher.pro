CONFIG += testcase parallel_test
TARGET = tst_qstringmatcher
QT = core testlib
SOURCES = tst_qstringmatcher.cpp
DEFINES += QT_NO_CAST_TO_ASCII
DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0
