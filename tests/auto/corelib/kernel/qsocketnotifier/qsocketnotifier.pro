CONFIG += testcase
CONFIG += parallel_test
TARGET = tst_qsocketnotifier
QT = core-private network-private testlib
SOURCES = tst_qsocketnotifier.cpp

requires(contains(QT_CONFIG,private_tests))

include(../../../network/socket/platformsocketengine/platformsocketengine.pri)
DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0
