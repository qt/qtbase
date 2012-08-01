CONFIG += testcase
TARGET = tst_modeltest
QT += widgets testlib
SOURCES         += tst_modeltest.cpp modeltest.cpp dynamictreemodel.cpp
HEADERS         += modeltest.h dynamictreemodel.h



DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0
