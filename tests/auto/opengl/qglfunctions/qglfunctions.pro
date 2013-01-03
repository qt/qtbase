CONFIG += testcase
CONFIG += parallel_test
TARGET = tst_qglfunctions
requires(qtHaveModule(opengl))
QT += opengl widgets testlib

SOURCES += tst_qglfunctions.cpp

win32:CONFIG+=insignificant_test # QTBUG-26390
DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0
