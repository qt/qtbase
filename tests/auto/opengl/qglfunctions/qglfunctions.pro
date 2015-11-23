CONFIG += testcase
TARGET = tst_qglfunctions
requires(qtHaveModule(opengl))
QT += opengl widgets testlib

SOURCES += tst_qglfunctions.cpp
