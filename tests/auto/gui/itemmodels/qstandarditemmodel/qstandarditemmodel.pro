CONFIG += testcase
TARGET = tst_qstandarditemmodel

QT += widgets widgets-private testlib
QT += core-private gui-private

mtdir = ../../../other/modeltest

INCLUDEPATH += $${mtdir}

SOURCES  += $${mtdir}/modeltest.cpp tst_qstandarditemmodel.cpp
HEADERS  += $${mtdir}/modeltest.h

