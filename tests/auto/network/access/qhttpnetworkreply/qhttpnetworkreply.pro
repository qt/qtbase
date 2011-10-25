CONFIG += testcase
TARGET = tst_qhttpnetworkreply
SOURCES  += tst_qhttpnetworkreply.cpp
INCLUDEPATH += $$QT_SOURCE_TREE/src/3rdparty/zlib
requires(contains(QT_CONFIG,private_tests))

QT = core-private network-private testlib
