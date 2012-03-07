CONFIG += testcase
TARGET = tst_qglfunctions
requires(contains(QT_CONFIG,opengl))
QT += opengl widgets testlib

SOURCES += tst_qglfunctions.cpp
