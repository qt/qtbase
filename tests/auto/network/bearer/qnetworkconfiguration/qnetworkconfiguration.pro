CONFIG += testcase
TARGET = tst_qnetworkconfiguration
SOURCES  += tst_qnetworkconfiguration.cpp
HEADERS  += ../qbearertestcommon.h

QT = core network testlib

maemo6|maemo5 {
    CONFIG += link_pkgconfig

    PKGCONFIG += conninet
}
