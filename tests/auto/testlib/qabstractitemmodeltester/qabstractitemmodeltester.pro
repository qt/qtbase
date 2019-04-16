CONFIG += testcase
QT += widgets testlib

mtdir = ../../other/qabstractitemmodelutils

INCLUDEPATH += $$PWD/$${mtdir}

SOURCES += \
  $${mtdir}/dynamictreemodel.cpp \
  tst_qabstractitemmodeltester.cpp

HEADERS += \
  $${mtdir}/dynamictreemodel.h

include($$QT_SOURCE_TREE/src/testlib/selfcover.pri)
