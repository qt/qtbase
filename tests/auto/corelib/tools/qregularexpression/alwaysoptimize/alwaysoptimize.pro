CONFIG += testcase parallel_test
TARGET = tst_qregularexpression_alwaysoptimize
QT = core testlib
HEADERS = ../tst_qregularexpression.h
SOURCES = \
    tst_qregularexpression_alwaysoptimize.cpp \
    ../tst_qregularexpression.cpp
DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0
