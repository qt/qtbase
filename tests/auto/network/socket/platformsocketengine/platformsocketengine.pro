CONFIG += testcase
TARGET = tst_platformsocketengine
SOURCES  += tst_platformsocketengine.cpp

include(../platformsocketengine/platformsocketengine.pri)

requires(contains(QT_CONFIG,private_tests))

MOC_DIR=tmp

QT = core-private network-private testlib
DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0
