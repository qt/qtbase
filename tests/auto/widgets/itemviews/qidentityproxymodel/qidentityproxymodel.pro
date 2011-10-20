load(qttest_p4)

mtdir = ../../../integrationtests/modeltest
INCLUDEPATH += $$PWD/$${mtdir}
QT += widgets
SOURCES         += tst_qidentityproxymodel.cpp $${mtdir}/dynamictreemodel.cpp $${mtdir}/modeltest.cpp
HEADERS         += $${mtdir}/dynamictreemodel.h $${mtdir}/modeltest.h
