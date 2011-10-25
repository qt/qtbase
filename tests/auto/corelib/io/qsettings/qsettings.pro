CONFIG += testcase
TARGET = tst_qsettings

QT += core-private testlib

SOURCES  += tst_qsettings.cpp
RESOURCES += qsettings.qrc

win32-msvc*:LIBS += advapi32.lib

CONFIG += parallel_test
