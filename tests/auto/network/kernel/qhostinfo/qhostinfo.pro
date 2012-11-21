CONFIG += testcase
TARGET = tst_qhostinfo

SOURCES  += tst_qhostinfo.cpp

QT = core-private network-private testlib

wince*: {
  LIBS += ws2.lib
} else {
  win32:LIBS += -lws2_32
}

# needed for getaddrinfo with official MinGW
win32-g++*:DEFINES += _WIN32_WINNT=0x0501

linux-*:CONFIG+=insignificant_test    # QTBUG-23837 - test is unstable
DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0
