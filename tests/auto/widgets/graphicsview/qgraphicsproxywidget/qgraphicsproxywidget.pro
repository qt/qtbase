CONFIG += testcase
TARGET = tst_qgraphicsproxywidget

QT += widgets widgets-private testlib
QT += core-private gui-private

SOURCES  += tst_qgraphicsproxywidget.cpp

# ### fixme: QTBUG-20756 crashes on xcb
contains(QT_CONFIG,xcb):CONFIG+=insignificant_test
