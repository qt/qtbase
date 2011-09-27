load(qttest_p4)
SOURCES  += tst_qnetworkconfigurationmanager.cpp
HEADERS  += ../qbearertestcommon.h

QT = core network

maemo6|maemo5 {
    CONFIG += link_pkgconfig

    PKGCONFIG += conninet
}
