CONFIG += testcase
TARGET = tst_qidentityproxymodel

mtdir = ../../../other/qabstractitemmodelutils
INCLUDEPATH += $$PWD/$${mtdir}
QT += testlib
SOURCES         += tst_qidentityproxymodel.cpp $${mtdir}/dynamictreemodel.cpp
HEADERS         += $${mtdir}/dynamictreemodel.h
