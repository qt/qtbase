CONFIG += testcase
TARGET = tst_qgraphicsitem
QT += widgets widgets-private testlib
QT += core-private gui-private
SOURCES  += tst_qgraphicsitem.cpp
DEFINES += QT_NO_CAST_TO_ASCII

win32:!wince*:!winrt: LIBS += -luser32
DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0
