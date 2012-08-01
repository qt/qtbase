CONFIG += testcase
TARGET = tst_qnetworkconfigurationmanager
SOURCES  += tst_qnetworkconfigurationmanager.cpp
HEADERS  += ../qbearertestcommon.h

QT = core network testlib
DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0
