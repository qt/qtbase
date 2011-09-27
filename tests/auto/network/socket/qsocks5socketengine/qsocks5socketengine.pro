load(qttest_p4)
SOURCES  += tst_qsocks5socketengine.cpp


include(../platformsocketengine/platformsocketengine.pri)


MOC_DIR=tmp

QT = core-private network-private

requires(contains(QT_CONFIG,private_tests))
