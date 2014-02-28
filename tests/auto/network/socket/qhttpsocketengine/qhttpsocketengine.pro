CONFIG += testcase
TARGET = tst_qhttpsocketengine
SOURCES  += tst_qhttpsocketengine.cpp


include(../platformsocketengine/platformsocketengine.pri)

MOC_DIR=tmp

requires(contains(QT_CONFIG,private_tests))
QT = core-private network-private testlib

