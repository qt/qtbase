CONFIG += testcase
TARGET = tst_qsortfilterproxymodel

QT += gui widgets testlib
mtdir = ../../../integrationtests/modeltest

INCLUDEPATH += $$PWD/$${mtdir}
SOURCES         += tst_qsortfilterproxymodel.cpp $${mtdir}/dynamictreemodel.cpp $${mtdir}/modeltest.cpp
HEADERS         += $${mtdir}/dynamictreemodel.h $${mtdir}/modeltest.h
