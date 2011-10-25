CONFIG += testcase
TARGET = tst_qhttpnetworkconnection
SOURCES  += tst_qhttpnetworkconnection.cpp
INCLUDEPATH += $$QT_SOURCE_TREE/src/3rdparty/zlib
requires(contains(QT_CONFIG,private_tests))

QT = core-private network-private testlib
