CONFIG += testcase
TARGET = tst_platformsocketengine
SOURCES  += tst_platformsocketengine.cpp

include(../platformsocketengine/platformsocketengine.pri)

requires(qtConfig(private_tests))

MOC_DIR=tmp

QT = core-private network-private testlib
