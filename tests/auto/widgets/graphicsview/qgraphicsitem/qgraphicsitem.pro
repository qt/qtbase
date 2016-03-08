CONFIG += testcase
TARGET = tst_qgraphicsitem
QT += widgets widgets-private testlib
QT += core-private gui-private
SOURCES  += tst_qgraphicsitem.cpp
DEFINES += QT_NO_CAST_TO_ASCII

win32:!winrt: LIBS += -luser32
