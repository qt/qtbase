load(qttest_p4)
SOURCES  += tst_qhttpnetworkconnection.cpp
INCLUDEPATH += $$QT_SOURCE_TREE/src/3rdparty/zlib
requires(contains(QT_CONFIG,private_tests))

QT = core-private network-private

CONFIG+=insignificant_test  # QTBUG-20981, crashes sometimes
