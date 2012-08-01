CONFIG += testcase
TARGET = tst_qnetworkconfiguration
SOURCES  += tst_qnetworkconfiguration.cpp
HEADERS  += ../qbearertestcommon.h

QT = core network testlib
DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0
