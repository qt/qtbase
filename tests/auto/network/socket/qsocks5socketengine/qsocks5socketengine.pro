CONFIG += testcase
TARGET = tst_qsocks5socketengine
SOURCES  += tst_qsocks5socketengine.cpp


include(../platformsocketengine/platformsocketengine.pri)


MOC_DIR=tmp

QT = core-private network-private testlib

# QTBUG-23380 - udpTest failing on Ubuntu 11.10 x64
linux-*:system(". /etc/lsb-release && [ $DISTRIB_CODENAME = oneiric ]"):CONFIG += insignificant_test

requires(contains(QT_CONFIG,private_tests))
