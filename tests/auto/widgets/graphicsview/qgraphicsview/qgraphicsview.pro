CONFIG += testcase
TARGET = tst_qgraphicsview

QT += widgets widgets-private testlib
QT += core-private gui-private

SOURCES  += tst_qgraphicsview.cpp tst_qgraphicsview_2.cpp
DEFINES += QT_NO_CAST_TO_ASCII

# ### fixme: QTBUG-20756 crashes on xcb
contains(QT_CONFIG,xcb):CONFIG+=insignificant_test
