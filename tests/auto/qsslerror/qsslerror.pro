load(qttest_p4)

SOURCES += tst_qsslerror.cpp
!wince*:win32:LIBS += -lws2_32
QT = core network

TARGET = tst_qsslerror

win32 {
  CONFIG(debug, debug|release) {
    DESTDIR = debug
} else {
    DESTDIR = release
  }
}

symbian: TARGET.CAPABILITY = NetworkServices

