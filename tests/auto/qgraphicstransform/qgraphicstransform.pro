load(qttest_p4)
QT += widgets
SOURCES  += tst_qgraphicstransform.cpp
CONFIG += parallel_test

linux-*:contains(QT_CONFIG,release):DEFINES+=MAY_HIT_QTBUG_20661
