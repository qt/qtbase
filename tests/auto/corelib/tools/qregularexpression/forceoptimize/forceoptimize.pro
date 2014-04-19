CONFIG += testcase parallel_test
TARGET = tst_qregularexpression_forceoptimize
QT = core testlib
HEADERS = ../tst_qregularexpression.h
SOURCES = \
    tst_qregularexpression_forceoptimize.cpp \
    ../tst_qregularexpression.cpp
DEFINES += forceOptimize=true
