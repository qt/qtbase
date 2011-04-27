load(qttest_p4)
SOURCES += tst_qsocketnotifier.cpp
QT = core network

requires(contains(QT_CONFIG,private_tests))

include(../qnativesocketengine/qsocketengine.pri)

symbian: TARGET.CAPABILITY = NetworkServices


