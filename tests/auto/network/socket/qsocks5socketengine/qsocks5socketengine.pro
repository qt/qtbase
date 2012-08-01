CONFIG += testcase
TARGET = tst_qsocks5socketengine
SOURCES  += tst_qsocks5socketengine.cpp


include(../platformsocketengine/platformsocketengine.pri)


MOC_DIR=tmp

QT = core-private network-private testlib

linux-*:system(". /etc/lsb-release && [ $DISTRIB_CODENAME = oneiric ]"):DEFINES+=UBUNTU_ONEIRIC # QTBUG-23380

requires(contains(QT_CONFIG,private_tests))
DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0
