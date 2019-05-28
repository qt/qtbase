CONFIG += testcase
TARGET = tst_qwindow

QT += core-private gui-private testlib

SOURCES  += tst_qwindow.cpp

qtConfig(dynamicgl):win32:!winrt: QMAKE_USE += user32
