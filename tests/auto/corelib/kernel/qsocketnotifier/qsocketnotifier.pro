CONFIG += testcase
TARGET = tst_qsocketnotifier
QT = core-private network-private testlib
SOURCES = tst_qsocketnotifier.cpp

requires(qtConfig(private_tests))

include(../../../network/socket/platformsocketengine/platformsocketengine.pri)
