load(qttest_p4)
SOURCES  += tst_qsocks5socketengine.cpp


include(../platformsocketengine/platformsocketengine.pri)


MOC_DIR=tmp

QT = core network

# Symbian toolchain does not support correct include semantics
symbian:INCLUDEPATH+=..\\..\\..\\include\\QtNetwork\\private
symbian: TARGET.CAPABILITY = NetworkServices


requires(contains(QT_CONFIG,private_tests))
