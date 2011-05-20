load(qttest_p4)
SOURCES  += tst_qhttpsocketengine.cpp


include(../platformsocketengine/platformsocketengine.pri)

MOC_DIR=tmp

QT = core-private network-private

symbian: TARGET.CAPABILITY = NetworkServices


