CONFIG += testcase
TARGET = tst_qgraphicssceneindex
requires(contains(QT_CONFIG,private_tests))
QT += widgets widgets-private testlib
QT += core-private gui-private
SOURCES  += tst_qgraphicssceneindex.cpp
CONFIG += parallel_test
DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0
