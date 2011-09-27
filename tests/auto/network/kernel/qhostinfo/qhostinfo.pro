load(qttest_p4)

SOURCES  += tst_qhostinfo.cpp

QT = core-private network-private

wince*: {
  LIBS += ws2.lib
} else {
  win32:LIBS += -lws2_32
}
