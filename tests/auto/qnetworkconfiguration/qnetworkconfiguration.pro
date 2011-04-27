load(qttest_p4)
SOURCES  += tst_qnetworkconfiguration.cpp
HEADERS  += ../qbearertestcommon.h

QT = core network

symbian {
    TARGET.CAPABILITY = NetworkServices NetworkControl ReadUserData 
}

maemo6|maemo5 {
    CONFIG += link_pkgconfig

    PKGCONFIG += conninet
}
