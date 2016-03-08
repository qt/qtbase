CONFIG += testcase
TARGET = tst_qwindow

QT += core-private gui-private testlib

SOURCES  += tst_qwindow.cpp

contains(QT_CONFIG,dynamicgl):win32:!winrt: LIBS += -luser32
