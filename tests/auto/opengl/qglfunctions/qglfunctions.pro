CONFIG += testcase
CONFIG += parallel_test
TARGET = tst_qglfunctions
requires(contains(QT_CONFIG,opengl))
QT += opengl widgets testlib

SOURCES += tst_qglfunctions.cpp

win32:CONFIG+=insignificant_test # QTBUG-26390
