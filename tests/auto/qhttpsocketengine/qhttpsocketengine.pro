load(qttest_p4)
SOURCES  += tst_qhttpsocketengine.cpp


include(../platformsocketengine/platformsocketengine.pri)

MOC_DIR=tmp

QT = core network

symbian: TARGET.CAPABILITY = NetworkServices


