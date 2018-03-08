CONFIG += testcase
TARGET = tst_qabstractitemmodel
QT = core testlib gui

mtdir = ../../../other/qabstractitemmodelutils
INCLUDEPATH += $$PWD/$${mtdir}
SOURCES = tst_qabstractitemmodel.cpp $${mtdir}/dynamictreemodel.cpp
HEADERS = $${mtdir}/dynamictreemodel.h
DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0
