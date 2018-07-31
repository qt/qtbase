CONFIG += testcase
TARGET = tst_qsortfilterproxymodel_regexp

QT += widgets testlib
mtdir = ../../../other/qabstractitemmodelutils
qsfpmdir = ../qsortfilterproxymodel_common

INCLUDEPATH += $$PWD/$${mtdir} $$PWD/$${qsfpmdir}
SOURCES += \
    tst_qsortfilterproxymodel_regexp.cpp \
    $${qsfpmdir}/tst_qsortfilterproxymodel.cpp \
    $${mtdir}/dynamictreemodel.cpp

HEADERS += \
    $${qsfpmdir}/tst_qsortfilterproxymodel.h \
    $${mtdir}/dynamictreemodel.h
