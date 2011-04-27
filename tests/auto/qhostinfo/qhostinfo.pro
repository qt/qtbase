load(qttest_p4)

SOURCES  += tst_qhostinfo.cpp

QT = core network

wince*: {
  LIBS += ws2.lib
} else {
  win32:LIBS += -lws2_32
}

symbian: TARGET.CAPABILITY = NetworkServices
symbian: {
  INCLUDEPATH *= $$MW_LAYER_SYSTEMINCLUDE
}
