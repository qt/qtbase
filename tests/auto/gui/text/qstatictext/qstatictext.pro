CONFIG += testcase
TARGET = tst_qstatictext
QT += widgets widgets-private testlib
QT += core core-private gui gui-private
SOURCES  += tst_qstatictext.cpp

CONFIG += insignificant_test   # QTBUG-21290 - crashes on qpa, xcb
