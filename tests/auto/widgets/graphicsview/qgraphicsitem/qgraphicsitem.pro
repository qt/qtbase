CONFIG += testcase
TARGET = tst_qgraphicsitem
QT += widgets widgets-private testlib
QT += core-private gui-private
SOURCES  += tst_qgraphicsitem.cpp
DEFINES += QT_NO_CAST_TO_ASCII

win32:!wince*: LIBS += -lUser32

# ### fixme: QTBUG-20756 crashes on xcb
contains(QT_CONFIG,xcb):CONFIG+=insignificant_test
