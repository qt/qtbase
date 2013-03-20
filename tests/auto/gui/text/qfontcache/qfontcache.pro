CONFIG += testcase
CONFIG += parallel_test
TARGET = tst_qfontcache
QT += testlib
QT += core-private gui-private
SOURCES  += tst_qfontcache.cpp

DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0
