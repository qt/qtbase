CONFIG += testcase
TARGET = tst_qhttpsocketengine
SOURCES  += tst_qhttpsocketengine.cpp


include(../platformsocketengine/platformsocketengine.pri)

MOC_DIR=tmp

QT = core-private network-private testlib

DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0
