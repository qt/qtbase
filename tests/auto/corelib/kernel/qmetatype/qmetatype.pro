CONFIG += testcase parallel_test
TARGET = tst_qmetatype
QT = core testlib
SOURCES = tst_qmetatype.cpp
TESTDATA=./typeFlags.bin
DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0
