CONFIG += testcase
TARGET = tst_qgraphicssceneindex
requires(contains(QT_CONFIG,private_tests))
QT += widgets widgets-private testlib
QT += core-private gui-private
SOURCES  += tst_qgraphicssceneindex.cpp
