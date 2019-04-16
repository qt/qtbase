CONFIG += testcase
TARGET = tst_qhttpnetworkconnection
SOURCES  += tst_qhttpnetworkconnection.cpp
requires(qtConfig(private_tests))

QT = core-private network-private testlib

QT_TEST_SERVER_LIST = apache2
include($$dirname(_QMAKE_CONF_)/tests/auto/testserver.pri)

