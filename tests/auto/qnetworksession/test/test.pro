load(qttest_p4)
SOURCES  += tst_qnetworksession.cpp
HEADERS  += ../../qbearertestcommon.h

QT = core network

TARGET = tst_qnetworksession
CONFIG(debug_and_release) {
  CONFIG(debug, debug|release) {
    DESTDIR = ../debug
  } else {
    DESTDIR = ../release
  }
} else {
  DESTDIR = ..
}

symbian {
    TARGET.CAPABILITY = NetworkServices NetworkControl ReadUserData PowerMgmt
}

maemo6|maemo5 {
    CONFIG += link_pkgconfig

    PKGCONFIG += conninet
}
