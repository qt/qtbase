CONFIG += testcase
TARGET = tst_qabstractitemmodel

mtdir = ../../../integrationtests/modeltest
INCLUDEPATH += $$PWD/$${mtdir}
QT += widgets testlib
SOURCES         += tst_qabstractitemmodel.cpp $${mtdir}/dynamictreemodel.cpp $${mtdir}/modeltest.cpp
HEADERS         += $${mtdir}/dynamictreemodel.h $${mtdir}/modeltest.h
