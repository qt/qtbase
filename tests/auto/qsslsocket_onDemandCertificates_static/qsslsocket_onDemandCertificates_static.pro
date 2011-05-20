load(qttest_p4)

SOURCES += tst_qsslsocket_onDemandCertificates_static.cpp
!wince*:win32:LIBS += -lws2_32
QT += core-private network-private
QT -= gui

TARGET = tst_qsslsocket_onDemandCertificates_static

win32 {
  CONFIG(debug, debug|release) {
    DESTDIR = debug
} else {
    DESTDIR = release
  }
}

wince* {
    DEFINES += SRCDIR=\\\"./\\\"
} else:symbian {
    TARGET.EPOCHEAPSIZE="0x100 0x1000000"
    TARGET.CAPABILITY=NetworkServices ReadUserData
    INCLUDEPATH *= $$MW_LAYER_SYSTEMINCLUDE  # Needed for e32svr.h in S^3 envs
} else {
    DEFINES += SRCDIR=\\\"$$PWD/\\\"
}

requires(contains(QT_CONFIG,private_tests))
