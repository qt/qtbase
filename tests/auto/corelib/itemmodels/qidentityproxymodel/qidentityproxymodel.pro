CONFIG += testcase
TARGET = tst_qidentityproxymodel

mtdir = ../../../other/modeltest
INCLUDEPATH += $$PWD/$${mtdir}
QT += widgets testlib
SOURCES         += tst_qidentityproxymodel.cpp $${mtdir}/dynamictreemodel.cpp $${mtdir}/modeltest.cpp
HEADERS         += $${mtdir}/dynamictreemodel.h $${mtdir}/modeltest.h
