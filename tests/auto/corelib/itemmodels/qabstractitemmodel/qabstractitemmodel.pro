CONFIG += testcase
CONFIG += parallel_test
TARGET = tst_qabstractitemmodel
QT += testlib

mtdir = ../../../other/modeltest
INCLUDEPATH += $$PWD/$${mtdir}
SOURCES = tst_qabstractitemmodel.cpp $${mtdir}/dynamictreemodel.cpp $${mtdir}/modeltest.cpp
HEADERS = $${mtdir}/dynamictreemodel.h $${mtdir}/modeltest.h
DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0
