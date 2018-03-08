CONFIG += testcase
TARGET = tst_qsortfilterproxymodel

QT += widgets testlib
mtdir = ../../../other/qabstractitemmodelutils

INCLUDEPATH += $$PWD/$${mtdir}
SOURCES         += tst_qsortfilterproxymodel.cpp $${mtdir}/dynamictreemodel.cpp
HEADERS         += $${mtdir}/dynamictreemodel.h
