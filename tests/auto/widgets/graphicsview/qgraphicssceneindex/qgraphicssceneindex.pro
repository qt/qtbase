CONFIG += testcase
TARGET = tst_qgraphicssceneindex
requires(qtConfig(private_tests))
QT += widgets widgets-private testlib
QT += core-private gui-private
SOURCES  += tst_qgraphicssceneindex.cpp
