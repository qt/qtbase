CONFIG += testcase parallel_test
TARGET = tst_qhash
QT = core testlib
SOURCES = $$PWD/tst_qhash.cpp
DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0
