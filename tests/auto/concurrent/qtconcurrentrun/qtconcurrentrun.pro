CONFIG += testcase parallel_test
TARGET = tst_qtconcurrentrun
QT = core testlib concurrent
SOURCES = tst_qtconcurrentrun.cpp
DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0
