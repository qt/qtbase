CONFIG += testcase
CONFIG += parallel_test
TARGET = tst_qsortfilterproxymodel

QT += widgets testlib
mtdir = ../../../other/modeltest

INCLUDEPATH += $$PWD/$${mtdir}
SOURCES         += tst_qsortfilterproxymodel.cpp $${mtdir}/dynamictreemodel.cpp $${mtdir}/modeltest.cpp
HEADERS         += $${mtdir}/dynamictreemodel.h $${mtdir}/modeltest.h
DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0
