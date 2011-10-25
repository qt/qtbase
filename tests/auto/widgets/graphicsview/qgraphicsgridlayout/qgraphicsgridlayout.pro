CONFIG += testcase
TARGET = tst_qgraphicsgridlayout

QT += widgets testlib
SOURCES  += tst_qgraphicsgridlayout.cpp
CONFIG += parallel_test
contains(QT_CONFIG,xcb):qpa:CONFIG+=insignificant_test  # QTBUG-20756 crashes on qpa, xcb
