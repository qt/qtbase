load(qttest_p4)
SOURCES += tst_qsocketnotifier.cpp
QT = core-private network-private

requires(contains(QT_CONFIG,private_tests))

include(../platformsocketengine/platformsocketengine.pri)

symbian: TARGET.CAPABILITY = NetworkServices

CONFIG += insignificant_test # QTBUG-21204
