load(qttest_p4)
SOURCES  += tst_qhostaddress.cpp


QT = core network

win32: {
wince*: {
	LIBS += -lws2
} else {
	LIBS += -lws2_32
}
}

symbian: TARGET.CAPABILITY = NetworkServices
