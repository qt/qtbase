CONFIG += testcase
CONFIG += parallel_test
TARGET = tst_qlist
QT = core testlib
SOURCES = $$PWD/tst_qlist.cpp
DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0
