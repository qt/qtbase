CONFIG += testcase
TARGET = tst_qgraphicsanchorlayout
QT += widgets widgets-private testlib
QT += core-private gui-private
SOURCES  += tst_qgraphicsanchorlayout.cpp
CONFIG += parallel_test
DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0
