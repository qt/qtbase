CONFIG += testcase
TARGET = tst_qabstractitemmodel
QT = widgets testlib

mtdir = ../../../integrationtests/modeltest
INCLUDEPATH += $$PWD/$${mtdir}
SOURCES = tst_qabstractitemmodel.cpp $${mtdir}/dynamictreemodel.cpp $${mtdir}/modeltest.cpp
HEADERS = $${mtdir}/dynamictreemodel.h $${mtdir}/modeltest.h
