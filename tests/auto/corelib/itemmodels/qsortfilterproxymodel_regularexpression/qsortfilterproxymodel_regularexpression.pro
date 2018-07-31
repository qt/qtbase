CONFIG += testcase
TARGET = tst_qsortfilterproxymodel_regularexpression

QT += widgets testlib
mtdir = ../../../other/qabstractitemmodelutils
qsfpmdir = ../qsortfilterproxymodel_common

INCLUDEPATH += $$PWD/$${mtdir} $${qsfpmdir}
SOURCES += \
    tst_qsortfilterproxymodel_regularexpression.cpp \
    $${qsfpmdir}/tst_qsortfilterproxymodel.cpp \
    $${mtdir}/dynamictreemodel.cpp

HEADERS += \
    $${qsfpmdir}/tst_qsortfilterproxymodel.h \
    $${mtdir}/dynamictreemodel.h
