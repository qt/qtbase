CONFIG += testcase
TARGET = tst_qgraphicsgridlayout

QT += widgets testlib
SOURCES  += tst_qgraphicsgridlayout.cpp
CONFIG += parallel_test
# ### fixme: QTBUG-20756 crashes on xcb
contains(QT_CONFIG,xcb):CONFIG+=insignificant_test
