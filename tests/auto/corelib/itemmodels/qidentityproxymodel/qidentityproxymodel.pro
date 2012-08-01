CONFIG += testcase
CONFIG += parallel_test
TARGET = tst_qidentityproxymodel

mtdir = ../../../other/modeltest
INCLUDEPATH += $$PWD/$${mtdir}
QT += testlib
SOURCES         += tst_qidentityproxymodel.cpp $${mtdir}/dynamictreemodel.cpp $${mtdir}/modeltest.cpp
HEADERS         += $${mtdir}/dynamictreemodel.h $${mtdir}/modeltest.h
DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0
