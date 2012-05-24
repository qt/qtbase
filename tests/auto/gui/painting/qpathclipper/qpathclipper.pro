CONFIG += testcase
CONFIG += parallel_test
TARGET = tst_qpathclipper
INCLUDEPATH += .
HEADERS += paths.h
SOURCES  += tst_qpathclipper.cpp paths.cpp
QT += gui-private testlib

requires(contains(QT_CONFIG,private_tests))

unix:!mac:LIBS+=-lm
