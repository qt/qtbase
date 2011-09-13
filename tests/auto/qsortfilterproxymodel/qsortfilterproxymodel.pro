load(qttest_p4)

QT += gui widgets

INCLUDEPATH += $$PWD/../modeltest

SOURCES         += tst_qsortfilterproxymodel.cpp ../modeltest/dynamictreemodel.cpp ../modeltest/modeltest.cpp
HEADERS         += ../modeltest/dynamictreemodel.h ../modeltest/modeltest.h

