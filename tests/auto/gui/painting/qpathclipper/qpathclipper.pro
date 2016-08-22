CONFIG += testcase
TARGET = tst_qpathclipper
INCLUDEPATH += .
HEADERS += paths.h
SOURCES  += tst_qpathclipper.cpp paths.cpp
QT += gui-private testlib

requires(qtConfig(private_tests))

unix:!darwin:!haiku:!integrity: LIBS += -lm
