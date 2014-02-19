isEmpty(TYPE): error("Project must define TYPE variable")

CONFIG += testcase parallel_test
QT = core testlib
TARGET = tst_qatomicinteger_$$TYPE
SOURCES = $$PWD/tst_qatomicinteger.cpp
DEFINES += QATOMIC_TEST_TYPE=$$TYPE tst_QAtomicIntegerXX=tst_QAtomicInteger_$$TYPE
