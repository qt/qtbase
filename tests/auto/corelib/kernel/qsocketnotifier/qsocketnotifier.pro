CONFIG += testcase
TARGET = tst_qsocketnotifier
SOURCES += tst_qsocketnotifier.cpp
QT = core-private network-private testlib

requires(contains(QT_CONFIG,private_tests))

include(../../../network/socket/platformsocketengine/platformsocketengine.pri)

CONFIG += insignificant_test # QTBUG-21204, QTBUG-21814
