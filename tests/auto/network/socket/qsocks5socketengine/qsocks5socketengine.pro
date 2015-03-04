CONFIG += testcase
TARGET = tst_qsocks5socketengine
SOURCES  += tst_qsocks5socketengine.cpp


include(../platformsocketengine/platformsocketengine.pri)


MOC_DIR=tmp

QT = core-private network-private testlib

requires(contains(QT_CONFIG,private_tests))
