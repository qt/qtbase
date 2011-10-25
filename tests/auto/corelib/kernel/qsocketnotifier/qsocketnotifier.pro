CONFIG += testcase
TARGET = tst_qsocketnotifier
SOURCES += tst_qsocketnotifier.cpp
QT = core-private network-private testlib

requires(contains(QT_CONFIG,private_tests))

include(../../../network/socket/platformsocketengine/platformsocketengine.pri)
