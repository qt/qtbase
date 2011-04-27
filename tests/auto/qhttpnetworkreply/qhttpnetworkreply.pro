load(qttest_p4)
SOURCES  += tst_qhttpnetworkreply.cpp
INCLUDEPATH += $$QT_SOURCE_TREE/src/3rdparty/zlib
requires(contains(QT_CONFIG,private_tests))

QT = core network
symbian: TARGET.CAPABILITY = NetworkServices
