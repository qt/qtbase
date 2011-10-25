CONFIG += testcase
TARGET = tst_qhostinfo

SOURCES  += tst_qhostinfo.cpp

QT = core-private network-private testlib

wince*: {
  LIBS += ws2.lib
} else {
  win32:LIBS += -lws2_32
}
